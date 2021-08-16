#include "floppy_disk.h"
#include "../interrupt.h"
#include "../peripherals.h"
#include "stdbool.h"
#include "../syslog.h"
#include "../asm.h"
#include "assert.h"
#include "errno.h"
#include "string.h"

//TODO: thread lock
//MAYBE: secondary floppy

/** Code adapted from OSDev forum. Really suboptimal */
/******************************************************************************
* Copyright (c) 2007 Teemu Voipio                                             *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL    *
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
******************************************************************************/

#define FLOPPY_BASE 0x03f0

// we statically reserve a totally uncomprehensive amount of memory
// must be large enough for whatever DMA transfer we might desire
// and must not cross 64k borders so easiest thing is to align it
// to 2^N boundary at least as big as the block
extern char floppy_dmabuf[0x4800];
static volatile unsigned floppy_dmacyl = -1;
static volatile bool floppy_dmawritten = false;

static volatile bool interrupted = false;
void floppy_IT() {
  io_write8(0x20, 0x20);
  interrupted = true;
}

enum registers {
  /** digital output register */
  DOR  = 2,
  /** master status register, read only */
  MSR  = 4,
  /** data FIFO, in DMA operation for commands */
  FIFO = 5,
  /** configuration control register, write only */
  CCR  = 7 
};

enum commands {
  CMD_SPECIFY = 3,
  CMD_WRITE_DATA = 5,
  CMD_READ_DATA = 6,
  CMD_RECALIBRATE = 7,
  CMD_SENSE_INTERRUPT = 8,
  CMD_SEEK = 15
};

enum floppy_type {
  FLOPPY_NONE,
  FLOPPY_36,
  FLOPPY_102,
  FLOPPY_72,

  FLOPPY_144,
  FLOPPY_288,
  FLOPPY_UNKNOWN
};
static enum floppy_type floppy_type = FLOPPY_UNKNOWN;

static uint8_t floppy_spt[] = {
    0,
    9,
    15,
    9,

    18,
    36,
    0
};

#define FLOPPY_SECTORS_PER_TRACK floppy_spt[floppy_type]
#define FLOPPY_SECTOR_SIZE 512
#define FLOPPY_CYLINDERS (floppy_type == FLOPPY_36 ? 40 : 80)
#define FLOPPY_CYLINDER_FACE_SIZE (FLOPPY_SECTORS_PER_TRACK * FLOPPY_SECTOR_SIZE)
#define FLOPPY_HEADS 2
#define FLOPPY_CYLINDER_SIZE (FLOPPY_CYLINDER_FACE_SIZE * FLOPPY_HEADS)
#define FLOPPY_GAP3 (floppy_type == FLOPPY_36 || floppy_type == FLOPPY_72 ? 0x2a : 0x1b)

//
// The MSR byte: [read-only]
// -------------
//
//  7   6   5    4    3    2    1    0
// MRQ DIO NDMA BUSY ACTD ACTC ACTB ACTA
//
// MRQ is 1 when FIFO is ready (test before read/write)
// DIO tells if controller expects write (1) or read (0)
//
// NDMA tells if controller is in DMA mode (1 = no-DMA, 0 = DMA)
// BUSY tells if controller is executing a command (1=busy)
//
// ACTA, ACTB, ACTC, ACTD tell which drives position/calibrate (1=yes)
//
//

static int flush();

/** Sleep delay in ms */
static void timer_sleep(int ms) {
    // FIXME: actual wait
    (void)ms;
    preempt_force();
}

static inline void wait_interrupt() {
  while (!interrupted) timer_sleep(10);
}

static void write_cmd(int base, char cmd) {
  int i; // do timeout, 60 seconds
  for(i = 0; i < 600; i++) {
    timer_sleep(10);
    if(0x80 & io_read8(base+MSR)) {
      io_write8(base+FIFO, cmd);
      return;
    }
  }
  assert(0);
}
static unsigned char read_data(int base) {
  int i; // do timeout, 60 seconds
  for(i = 0; i < 600; i++) {
    timer_sleep(10);
    if(0x80 & io_read8(base+MSR)) {
      return io_read8(base+FIFO);
    }
  }
  assert(0);
  return 0; // not reached
}


//
// The DOR byte: [write-only]
// -------------
//
//  7    6    5    4    3   2    1   0
// MOTD MOTC MOTB MOTA DMA NRST DR1 DR0
//
// DR1 and DR0 together select "current drive" = a/00, b/01, c/10, d/11
// MOTA, MOTB, MOTC, MOTD control motors for the four drives (1=on)
//
// DMA line enables (1 = enable) interrupts and DMA
// NRST is "not reset" so controller is enabled when it's 1
//
enum { motor_off = 0, motor_on, motor_wait };
static volatile int motor_ticks = 0;
static volatile int motor_state = 0;

static void motor(int base, int onoff) {
  if(onoff) {
    if(!motor_state) {
      // need to turn on
      io_write8(base + DOR, 0x1c);
      timer_sleep(500); // wait 500 ms = hopefully enough for modern drives
    }
    motor_state = motor_on;
  } else {
    if(motor_state == motor_wait) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "motor: strange, fd motor-state already waiting..\n");
    }
    motor_ticks = 3000; // 3 seconds, see timer() below
    motor_state = motor_wait;
  }
}

static void motor_kill(int base) {
  io_write8(base + DOR, 0x0c);
  motor_state = motor_off;
}

static void sleeper_decr(int ms) {
  if(motor_state == motor_wait) {
    motor_ticks -= ms;
    if(motor_ticks <= 0) {
      flush();
      if (motor_ticks <= 0)
        motor_kill(FLOPPY_BASE);
    }
  }
}

#if FLOPPY_USE_THREAD
//
// THIS SHOULD BE STARTED IN A SEPARATE THREAD.
//
//
int floppy_sleeper_thread(void* arg) {
  (void)arg;
  while (1) {
    // sleep for 500ms at a time, which gives around half
    // a second jitter, but that's hardly relevant here.
    timer_sleep(500);
    sleeper_decr(500);
  }
}
#else
void floppy_sleeper_pull() {
  static unsigned long last_time = UINT32_MAX;
  unsigned long current_time = pit_get_count() * (1000 / PIT_FREQ);
  sleeper_decr(current_time - last_time);
  last_time = current_time;
}
#endif

static void check_interrupt(int base, int *st0, int *cyl) {
   
  write_cmd(base, CMD_SENSE_INTERRUPT);

  *st0 = read_data(base);
  *cyl = read_data(base);
}

#if 0
// Move to cylinder 0, which calibrates the drive..
static int calibrate(int base) {

  int i, st0, cyl = -1; // set to bogus cylinder

  motor(base, motor_on);

  for(i = 0; i < 10; i++) {
    interrupted = false;
    // Attempt to positions head to cylinder 0
    write_cmd(base, CMD_RECALIBRATE);
    write_cmd(base, 0); // argument is drive, we only support 0

    wait_interrupt();
    check_interrupt(base, &st0, &cyl);
    
    if(st0 & 0xC0) {
      static const char * status[] =
      { 0, "error", "invalid", "drive" };
      syslogf((syslog_ctx){"FLOPPY", NULL, WARN}, "calibrate: status = %s\n", status[st0 >> 6]);
      continue;
    }

    if(!cyl) { // found cylinder 0 ?
      motor(base, motor_off);
      return 0;
    }
  }

  syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "calibrate: 10 retries exhausted\n");
  motor(base, motor_off);
  return -1;
}
#endif

#if 0
static int reset(int base) {

  interrupted = false;
  io_write8(base + DOR, 0x00); // disable controller
  io_write8(base + DOR, 0x0C); // enable controller

  wait_interrupt(); // sleep until interrupt occurs

  {
    int st0, cyl; // ignore these here..
    check_interrupt(base, &st0, &cyl);
  }

  // set transfer speed 500kb/s
  io_write8(base + CCR, 0x00);

  //  - 1st byte is: bits[7:4] = steprate, bits[3:0] = head unload time
  //  - 2nd byte is: bits[7:1] = head load time, bit[0] = no-DMA
  //
  //  steprate    = (8.0ms - entry*0.5ms)*(1MB/s / xfer_rate)
  //  head_unload = 8ms * entry * (1MB/s / xfer_rate), where entry 0 -> 16
  //  head_load   = 1ms * entry * (1MB/s / xfer_rate), where entry 0 -> 128
  //
  write_cmd(base, CMD_SPECIFY);
  write_cmd(base, 0xdf); /* steprate = 3ms, unload time = 240ms */
  write_cmd(base, 0x02); /* load time = 16ms, no-DMA = 0 */

  // it could fail...
  if(calibrate(base)) return -1;
   
  return 0;
}
#endif

// Seek for a given cylinder, with a given head
static int seek(int base, unsigned cyli, int head) {

  unsigned i, st0, cyl = -1; // set to bogus cylinder

  motor(base, motor_on);

  for(i = 0; i < 10; i++) {
    interrupted = false;
    // Attempt to position to given cylinder
    // 1st byte bit[1:0] = drive, bit[2] = head
    // 2nd byte is cylinder number
    write_cmd(base, CMD_SEEK);
    write_cmd(base, head<<2);
    write_cmd(base, cyli);

    wait_interrupt();
    check_interrupt(base, (int*)&st0, (int*)&cyl);

    if(st0 & 0xC0) {
      static const char * status[] =
      { "normal", "error", "invalid", "drive" };
      syslogf((syslog_ctx){"FLOPPY", NULL, WARN}, "seek: status = %s\n", status[st0 >> 6]);
      continue;
    }

    if(cyl == cyli) {
      motor(base, motor_off);
      return 0;
    }

  }

  syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "seek: 10 retries exhausted\n");
  motor(base, motor_off);
  return -1;
}

// Used by dma_init and do_track to specify direction
typedef enum {
    dir_read = 1,
    dir_write = 2
} dir;


static void dma_init(dir dir) {

  union {
    unsigned char b[4]; // 4 bytes
    unsigned long l;    // 1 long = 32-bit
  } a, c; // address and count

  a.l = (unsigned) &floppy_dmabuf;
  c.l = (unsigned) FLOPPY_CYLINDER_SIZE - 1;  // -1 because of DMA counting

  // check that address is at most 24-bits (under 16MB)
  // check that count is at most 16-bits (DMA limit)
  // check that if we add count and address we don't get a carry
  // (DMA can't deal with such a carry, this is the 64k boundary limit)
  if((a.l >> 24) || (c.l >> 16) || (((a.l&0xffff)+c.l)>>16)) {
    panic("dma_init: static buffer problem\n");
  }

  unsigned char mode;
  switch(dir) {
    // 01:0:0:01:10 = single/inc/no-auto/to-mem/chan2
    case dir_read:  mode = 0x46; break;
    // 01:0:0:10:10 = single/inc/no-auto/from-mem/chan2
    case dir_write: mode = 0x4a; break;
    default: panic("dma_init: invalid direction");
              return; // not reached, please "mode user uninitialized"
  }

  io_write8(0x0a, 0x06);   // mask chan 2

  io_write8(0x0c, 0xff);   // reset flip-flop
  io_write8(0x04, a.b[0]); //  - address low byte
  io_write8(0x04, a.b[1]); //  - address high byte

  io_write8(0x81, a.b[2]); // external page register

  io_write8(0x0c, 0xff);   // reset flip-flop
  io_write8(0x05, c.b[0]); //  - count low byte
  io_write8(0x05, c.b[1]); //  - count high byte

  io_write8(0x0b, mode);   // set mode (see above)

  io_write8(0x0a, 0x02);   // unmask chan 2
}

// This monster does full cylinder (both tracks) transfer to
// the specified direction (since the difference is small).
//
// It retries (a lot of times) on all errors except write-protection
// which is normally caused by mechanical switch on the disk.
//
static int do_track(int base, unsigned cyl, dir dir) {
   
  // transfer command, set below
  unsigned char cmd;

  // Read is MT:MF:SK:0:0:1:1:0, write MT:MF:0:0:1:0:1
  // where MT = multitrack, MF = MFM mode, SK = skip deleted
  //
  // Specify multitrack and MFM mode
  static const int flags = 0xC0;
  switch(dir) {
    case dir_read:
        cmd = CMD_READ_DATA | flags;
        break;
    case dir_write:
        cmd = CMD_WRITE_DATA | flags;
        break;
    default:

        panic("do_track: invalid direction");
        return 0; // not reached, but pleases "cmd used uninitialized"
  }

  // seek both heads
  if(seek(base, cyl, 0)) return -1;
  if(seek(base, cyl, 1)) return -1;

  int i;
  for(i = 0; i < 20; i++) {
    motor(base, motor_on);

    // init dma..
    dma_init(dir);

    timer_sleep(100); // give some time (100ms) to settle after the seeks

    interrupted = false;

    write_cmd(base, cmd);  // set above for current direction
    write_cmd(base, 0);    // 0:0:0:0:0:HD:US1:US0 = head and drive
    write_cmd(base, cyl);  // cylinder
    write_cmd(base, 0);    // first head (should match with above)
    write_cmd(base, 1);    // first sector, strangely counts from 1
    write_cmd(base, 2);    // bytes/sector, 128*2^x (x=2 -> 512)
    write_cmd(base, FLOPPY_SECTORS_PER_TRACK);   // number of tracks to operate on
    write_cmd(base, FLOPPY_GAP3); // GAP3 length, 27 is default for 3.5"
    write_cmd(base, 0xff); // data length (0xff if B/S != 0)
    
    wait_interrupt(); // don't SENSE_INTERRUPT here!

    // first read status information
    unsigned char st0, st1, st2, rcy, rhe, rse, bps;
    st0 = read_data(base);
    st1 = read_data(base);
    st2 = read_data(base);
    /*
      * These are cylinder/head/sector values, updated with some
      * rather bizarre logic, that I would like to understand.
      *
      */
    rcy = read_data(base);
    rhe = read_data(base);
    rse = read_data(base);
    // bytes per sector, should be what we programmed in
    bps = read_data(base);

    (void)rse;
    (void)rhe;
    (void)rcy;

    int error = 0;

    if(st0 & 0xC0) {
      static const char * status[] =
      { 0, "error", "invalid command", "drive not ready" };
      syslogf((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: status = %s\n", status[st0 >> 6]);
      error = 1;
    }
    if(st1 & 0x80) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: end of cylinder\n");
      error = 1;
    }
    if(st0 & 0x08) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: drive not ready\n");
      error = 1;
    }
    if(st1 & 0x20) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: CRC error\n");
      error = 1;
    }
    if(st1 & 0x10) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: controller timeout\n");
      error = 1;
    }
    if(st1 & 0x04) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: no data found\n");
      error = 1;
    }
    if((st1|st2) & 0x01) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: no address mark found\n");
      error = 1;
    }
    if(st2 & 0x40) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: deleted address mark\n");
      error = 1;
    }
    if(st2 & 0x20) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: CRC error in data\n");
      error = 1;
    }
    if(st2 & 0x10) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: wrong cylinder\n");
      error = 1;
    }
    if(st2 & 0x04) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: uPD765 sector not found\n");
      error = 1;
    }
    if(st2 & 0x02) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: bad cylinder\n");
      error = 1;
    }
    if(bps != 0x2) {
      syslogf((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: wanted 512B/sector, got %d", (1<<(bps+7)));
      error = 1;
    }
    if(st1 & 0x02) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: not writable\n");
      error = 2;
    }

    if(!error) {
      motor(base, motor_off);
      return 0;
    }
    if(error > 1) {
      syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: not retrying..\n");
      motor(base, motor_off);
      return -2;
    }
  }

  syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "do_sector: 20 retries exhausted\n");
  motor(base, motor_off);
  return -1;

}

static inline void addr_2_coff(uint32_t addr, uint16_t* cyl, uint32_t* offset, uint32_t* size) {
  if (offset) *offset = addr % FLOPPY_CYLINDER_SIZE;
  if (size) *size = FLOPPY_CYLINDER_SIZE - (addr % FLOPPY_CYLINDER_SIZE);
  if (cyl) *cyl = addr / FLOPPY_CYLINDER_SIZE;
}

static int flush() {
  if (floppy_dmawritten) {
    int res = do_track(FLOPPY_BASE, floppy_dmacyl, dir_write);
    if (res >= 0) floppy_dmawritten = false;
    return res;
  }
  return 1;
}
static int do_cached(dir dir, uint32_t addr, uint32_t* offset, uint32_t* size) {
  uint16_t cyl;
  addr_2_coff(addr, &cyl, offset, size);
  int ret = 1;
  if (cyl != floppy_dmacyl) {
    assert(dir == dir_read);
    flush();
    ret = do_track(FLOPPY_BASE, cyl, dir);
    if (ret >= 0) floppy_dmacyl = cyl;
  }
  floppy_dmawritten |= (dir == dir_write);
  return ret;
}

static int floppy_read(disk* d, void* out, sector_n count, sector_n offset) {
    if (count + offset > FLOPPY_SECTORS_PER_TRACK * FLOPPY_CYLINDERS * FLOPPY_HEADS)
        return -ERANGE;
    
    uint32_t off, working_size;
    // Reading cylinders one by one
    for (uint32_t done_size = 0; done_size < count * FLOPPY_SECTOR_SIZE; done_size += working_size) {
        int ret = do_cached(dir_read, offset * FLOPPY_SECTOR_SIZE + done_size, &off, &working_size);
        if (ret < 0) return ret;

        if (working_size > count * FLOPPY_SECTOR_SIZE - done_size)
            working_size = count * FLOPPY_SECTOR_SIZE - done_size;
        memcpy(out + done_size, &floppy_dmabuf[off], working_size);
    }
    return 1;
}
static int floppy_write(disk* d, const void* in, sector_n count, sector_n offset) {
    if (count + offset > FLOPPY_SECTORS_PER_TRACK * FLOPPY_CYLINDERS * FLOPPY_HEADS)
        return -ERANGE;
    
    uint32_t off, working_size;
    // Writing cylinders one by one
    for (uint32_t done_size = 0; done_size < count * FLOPPY_SECTOR_SIZE; done_size += working_size) {
        // Load cylinder
        int ret = do_cached(dir_read, offset * FLOPPY_SECTORS_PER_TRACK + done_size, &off, &working_size);
        if (ret < 0) return ret;

        if (working_size > count * FLOPPY_SECTOR_SIZE - done_size)
            working_size = count * FLOPPY_SECTOR_SIZE - done_size;
        // Edit buffer
        memcpy(&floppy_dmabuf[off], in + done_size, working_size);

        // Save cylinder
        ret = do_cached(dir_write, offset * FLOPPY_SECTORS_PER_TRACK + done_size, NULL, NULL);
        if (ret < 0) return ret;
    }
    return 1;
}
static int floppy_cmd(disk * d, enum disk_command cmd, void *io) {
    switch (cmd)
    {
    case DISK_GET_STATUS:
        if (!io)
            return -EINVAL;
        *(enum disk_status *)io = motor_state == motor_off ? DISK_OFF : 0;
        return 1;
    case DISK_SYNC:
        return flush();
    case DISK_GET_SECTOR_SIZE:
        if (!io)
            return -EINVAL;
        *(size_t *)io = FLOPPY_SECTOR_SIZE;
        return 1;
    case DISK_GET_SECTOR_COUNT:
        if (!io)
            return -EINVAL;
        *(size_t *)io = FLOPPY_SECTORS_PER_TRACK * FLOPPY_CYLINDERS * FLOPPY_HEADS;
        return 1;
    case DISK_GET_BLOCK_SIZE:
        if (!io)
            return -EINVAL;
        *(size_t *)io = FLOPPY_SECTORS_PER_TRACK;
        return 1;
    case DISK_TRIM:
        return -EPROTONOSUPPORT;
    case DISK_POWER:
        if (!io)
            return -EINVAL;
        if (*(bool*)io)
          motor(FLOPPY_BASE, motor_on);
        else
            motor_kill(FLOPPY_BASE);

        return 1;
    case DISK_EJECT:
        flush();
        d->write = NULL;
        d->read = NULL;
        d->cmd = NULL;
        //MAYBE: real eject
        return 1;
    default:
        return -EINVAL;
    }
}

disk* floppy_disk_get() {
    static disk self = {floppy_read, floppy_write, floppy_cmd};
    if (floppy_type == FLOPPY_UNKNOWN) {
        floppy_type = FLOPPY_NONE;
        // Read drives list in CMOS
        uint8_t main_drive = cmos_read(0x10) >> 4;
        if(main_drive == FLOPPY_288) {
            syslogs((syslog_ctx){"FLOPPY", NULL, WARN}, "2.88mb disk not supported");
        } else if (main_drive > FLOPPY_NONE && main_drive < FLOPPY_UNKNOWN) {
            floppy_type = main_drive;
        } else {
            syslogs((syslog_ctx){"FLOPPY", NULL, DEBUG}, "No disk found");
        }
    }
    return floppy_type != FLOPPY_NONE ? &self : NULL;
}
