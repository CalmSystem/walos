#include "e1000.h"
#include "../../memory.h"
#include "../../pci_driver.h"
#include "../etharp.h"
#include "../snmp.h"
#include "string.h"

/* Based on Joshua Cornutt i825xx and MIT students (gonglinyuan) JOS Lab3 e1000 drivers */

/* pci extension */
static uint32_t mmio_read32(void* addr) {
  return *((volatile uint32_t*)(addr));
}
static void mmio_write32(void* addr, uint32_t val) {
  (*((volatile uint32_t*)(addr))) = val;
}

static inline void command_write(struct ifstate* state, uint16_t addr, uint32_t val) {
  mmio_write32(state->mmio_addr + addr, val);
}
static inline uint32_t command_read(struct ifstate* state, uint16_t addr) {
  return mmio_read32(state->mmio_addr + addr);
}

/* e1000 driver (eX) */
#define E1000_REG_CTRL 0x0000
#define E1000_REG_STATUS 0x0008
#define E1000_REG_EEPROM 0x0014
#define E1000_REG_CTRL_EXT 0x0018
#define E1000_REG_ICR 0x00C0
#define E1000_REG_IMS 0x00D0

#define E1000_REG_RCTRL 0x0100
#define E1000_REG_RXDESCLO 0x2800
#define E1000_REG_RXDESCHI 0x2804
#define E1000_REG_RXDESCLEN 0x2808
#define E1000_REG_RXDESCHEAD 0x2810
#define E1000_REG_RXDESCTAIL 0x2818

#define E1000_REG_TCTRL 0x0400
#define E1000_REG_TXDESCLO 0x3800
#define E1000_REG_TXDESCHI 0x3804
#define E1000_REG_TXDESCLEN 0x3808
#define E1000_REG_TXDESCHEAD 0x3810
#define E1000_REG_TXDESCTAIL 0x3818

#define E1000_TXD_STAT_DD 0x0001
#define E1000_TXD_CMD_RS 0x0008
#define E1000_TXD_CMD_EOP 0x0001

#define E1000_REG_MTA 0x5200
#define E1000_REG_RXADDR 0x5400

#define E1000_CTRL_FD (1 << 0)
#define E1000_CTRL_ASDE (1 << 5)
#define E1000_CTRL_SLU (1 << 6)
#define E1000_TCTL_EN (1 << 1)
#define E1000_TCTL_PSP (1 << 3)
#define E1000_RCTL_EN (1 << 1)
#define E1000_RCTL_SBP (1 << 2)
#define E1000_RCTL_UPE (1 << 3)
#define E1000_RCTL_MPE (1 << 4)
#define E1000_RCTL_LPE (1 << 5)
#define E1000_RCTL_BAM (1 << 15)
#define E1000_RCTL_BSIZE_256 (3 << 16)
#define E1000_RCTL_BSIZE_512 (2 << 16)
#define E1000_RCTL_BSIZE_1024 (1 << 16)
#define E1000_RCTL_BSIZE_2048 (0 << 16)
#define E1000_RCTL_BSIZE_4096 ((3 << 16) | (1 << 25))
#define E1000_RCTL_BSIZE_8192 ((2 << 16) | (1 << 25))
#define E1000_RCTL_BSIZE_16384 ((1 << 16) | (1 << 25))
#define E1000_RCTL_SECRC (1 << 26)

/* Configuration constants */
#define E1000_CONF_TX_TDNUM 32
#define E1000_CONF_TX_TDLEN (E1000_CONF_TX_TDNUM * 16)
#define E1000_CONF_TX_BUFSIZE 2048
#define E1000_CONF_TX_BUF_PER_PAGE (PAGE_SIZE / E1000_CONF_TX_BUFSIZE)
#define E1000_CONF_RX_RDNUM 256
#define E1000_CONF_RX_RDLEN (E1000_CONF_RX_RDNUM * 16)
#define E1000_CONF_RX_BUFSIZE 2048
#define E1000_CONF_RX_BUF_PER_PAGE (PAGE_SIZE / E1000_CONF_RX_BUFSIZE)
#define E1000_CONF_RX_RAL_VAL 0x12005452
#define E1000_CONF_RX_RAH_VAL 0x00005634

static int e1000_has_eeprom(struct ifstate* state) {
  command_write(state, E1000_REG_EEPROM, 1);
  for (int i = 0; i < 100000; i++) {
    uint32_t val = command_read(state, E1000_REG_EEPROM);
    if (val & 0x10) return 1;
  }
  return 0;
}
static uint16_t e1000_eeprom_read(struct ifstate* state, uint8_t addr) {
  uint32_t tmp = 0;
  command_write(state, E1000_REG_EEPROM, 1 | ((uint32_t)(addr) << 8));
  while (!((tmp = command_read(state, E1000_REG_EEPROM)) & (1 << 4)));
  return (uint16_t)((tmp >> 16) & 0xFFFF);
}

struct e1000_rx_desc_t {
  volatile uint64_t address;
  volatile uint16_t length;
  volatile uint16_t checksum;
  volatile uint8_t status;
  volatile uint8_t errors;
  volatile uint16_t special;
} __attribute__((packed));
struct e1000_tx_desc_t {
  volatile uint64_t address;
  volatile uint16_t length;
  volatile uint8_t cso;
  volatile uint8_t cmd;
  volatile uint8_t status;
  volatile uint8_t css;
  volatile uint16_t special;
} __attribute__((packed));

static err_t
e1000_init(struct netif *netif)
{
  struct ifstate *state = netif->state;
  const pci_id pciId = state->pci.pciId;

  pci_enable(pciId);
  // Wait startup... */

  struct pci_bar bar;
  /* read the PCI BAR for the device's MMIO space */
  pci_get_bar(&bar, pciId, 0);

  if (!bar.size || (bar.flags & PCI_BAR_IO))
    return ERR_IF;

  state->mmio_addr = bar.u.address;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  if (e1000_has_eeprom(state)) {
    uint16_t mac16;
    mac16 = e1000_eeprom_read(state, 0);
    netif->hwaddr[0] = (mac16 & 0xFF);
    netif->hwaddr[1] = (mac16 >> 8) & 0xFF;
    mac16 = e1000_eeprom_read(state, 1);
    netif->hwaddr[2] = (mac16 & 0xFF);
    netif->hwaddr[3] = (mac16 >> 8) & 0xFF;
    mac16 = e1000_eeprom_read(state, 2);
    netif->hwaddr[4] = (mac16 & 0xFF);
    netif->hwaddr[5] = (mac16 >> 8) & 0xFF;
    /*
    // Write back
    uint32_t mac_addr_low, mac_addr_low;
    memcpy(&mac_addr_low, &netif->hwaddr[0], 4);
    memcpy(&mac_addr_low, &netif->hwaddr[4], 2);
    memset((uint8_t *)&mac_addr_low + 2, 0, 2);
    mac_addr_low |= 0x80000000;
    command_write(state, E1000_REG_RXADDR + 0, mac_addr_low);
    command_write(state, E1000_REG_RXADDR + 4, mac_addr_low);*/
  } else {
    uint32_t mac_addr_low = *(uint32_t*)(state->mmio_addr + E1000_REG_RXADDR);
    uint32_t mac_addr_high = *(uint32_t*)(state->mmio_addr + E1000_REG_RXADDR + 4);
    netif->hwaddr[0] = (mac_addr_low >> 0) & 0xFF;
    netif->hwaddr[1] = (mac_addr_low >> 8) & 0xFF;
    netif->hwaddr[2] = (mac_addr_low >> 16) & 0xFF;
    netif->hwaddr[3] = (mac_addr_low >> 24) & 0xFF;
    netif->hwaddr[4] = (mac_addr_high >> 0) & 0xFF;
    netif->hwaddr[5] = (mac_addr_high >> 8) & 0xFF;
  }

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  // Set link up
  mmio_write32(E1000_REG_CTRL, (mmio_read32(E1000_REG_CTRL) | E1000_CTRL_SLU));

  { // Enable transmit
    LWIP_ASSERT("e1000: Too many tx descriptors", E1000_CONF_TX_TDNUM * sizeof(struct e1000_tx_desc_t) <= PAGE_SIZE && E1000_CONF_TX_TDNUM <= IF_TDBUFNUM);
    state->tx_desc = page_calloc(1);
    if (!state->tx_desc) return ERR_MEM;
    struct e1000_tx_desc_t* tx_desc = (void*)state->tx_desc;

    for (int i = 0; i < E1000_CONF_TX_TDNUM; i += E1000_CONF_TX_BUF_PER_PAGE) {
      void* pg = page_calloc(1);
      if (!pg) return ERR_MEM;

      for (int j = 0; j < E1000_CONF_TX_BUF_PER_PAGE; ++j) {
        state->tx_buf[i + j] = pg + j * E1000_CONF_TX_BUFSIZE;
        tx_desc[i + j].address = (uint64_t)pg + j * E1000_CONF_TX_BUFSIZE;
        tx_desc[i + j].length = E1000_CONF_TX_BUFSIZE;
        tx_desc[i + j].cmd |= E1000_TXD_CMD_RS;
        tx_desc[i + j].status |= E1000_TXD_STAT_DD;
      }
    }

    command_write(state, E1000_REG_TXDESCLO, (uint64_t)tx_desc & 0xFFFFFFFF);
    command_write(state, E1000_REG_TXDESCHI, (uint64_t)tx_desc >> 32);
    command_write(state, E1000_REG_TXDESCLEN, E1000_CONF_TX_TDLEN);
    command_write(state, E1000_REG_TXDESCHEAD, 0);
    command_write(state, E1000_REG_TXDESCTAIL, 0);

    command_write(state, E1000_REG_TCTRL, (E1000_TCTL_EN | E1000_TCTL_PSP));
  }

  { // Enable receive
    LWIP_ASSERT("e1000: Too many rx descriptors", E1000_CONF_RX_RDNUM * sizeof(struct e1000_rx_desc_t) <= PAGE_SIZE);
    state->rx_desc = page_calloc(1);
    if (!state->rx_desc) return ERR_MEM;
    struct e1000_tx_desc_t* rx_desc = (void*)state->rx_desc;

    for (int i = 0; i < E1000_CONF_RX_RDNUM; i += E1000_CONF_RX_BUF_PER_PAGE) {
      void* pg = page_calloc(1);
      if (!pg) return ERR_MEM;

      for (int j = 0; j < E1000_CONF_RX_BUF_PER_PAGE; ++j) {
        rx_desc[i + j].address = (uint64_t)pg + j * E1000_CONF_RX_BUFSIZE;
      }
    }

    for (int i = 0; i < 128; i++)
      command_write(state, E1000_REG_MTA + (i * 4), 0);

    // Disable interrupts
    command_write(state, E1000_REG_IMS, 0);

    command_write(state, E1000_REG_RXDESCLO, (uint64_t)rx_desc & 0xFFFFFFFF);
    command_write(state, E1000_REG_RXDESCHI, (uint64_t)rx_desc >> 32);
    command_write(state, E1000_REG_RXDESCLEN, E1000_CONF_TX_TDLEN);
    command_write(state, E1000_REG_RXDESCHEAD, 0);
    command_write(state, E1000_REG_RXDESCTAIL, E1000_CONF_RX_RDNUM - 1);

    command_write(state, E1000_REG_RCTRL, (E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC));
  }

  return ERR_OK;
}

/*static void e1000_down() {
  for rx += E1000_CONF_RX_BUF_PER_PAGE
    page_free(.addr)

  page_free(rx_desc)

  for rx += E1000_CONF_TX_BUF_PER_PAGE
    page_free(.addr)

  page_free(tx_desc)
}*/

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
e1000_output(struct netif *netif, struct pbuf *p)
{
  struct ifstate *state = netif->state;

  // initiate transfer();

#if ETH_PAD_SIZE
  pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

  uint32_t head = command_read(state, E1000_REG_TXDESCHEAD);
  uint32_t tail = command_read(state, E1000_REG_TXDESCTAIL);
  struct e1000_tx_desc_t *tx_desc = (void*)state->tx_desc;

  //MAYBE: in buffer packing
  // Check buffer fits
  uint32_t vtail = tail;
  for (struct pbuf *q = p; q != NULL; q = q->next) {
    for (uint16_t start = 0; start < q->len; start += E1000_CONF_TX_BUFSIZE) {
      if ((vtail + 1) % E1000_CONF_TX_TDNUM == head ||
          !(tx_desc[vtail].status & E1000_TXD_STAT_DD)
      ) {
        MIB2_STATS_NETIF_INC(netif, ifouterrors);
        return ERR_BUF;
      }

      vtail = (vtail + 1) % E1000_CONF_TX_TDNUM;
    }
  }

  for (struct pbuf *q = p; q != NULL; q = q->next) {
    for (uint16_t start = 0; start < q->len; start += E1000_CONF_TX_BUFSIZE) {
      int end_of_part = q->len - start <= E1000_CONF_TX_BUFSIZE;
      uint16_t len = end_of_part ? E1000_CONF_TX_BUFSIZE : q->len - start;
      memmove(state->tx_buf[tail], q->payload + start, len);
      tx_desc[tail].address = (uint64_t)state->tx_buf[tail];
      tx_desc[tail].length = len;
      tx_desc[tail].cmd = E1000_TXD_CMD_RS | (end_of_part && q->next == NULL ?
        E1000_TXD_CMD_EOP : 0);
      tx_desc[tail].status = 0;

      tail = (tail + 1) % E1000_CONF_TX_TDNUM;
    }
  }

  // signal that packet should be sent();
  command_write(state, E1000_REG_TXDESCTAIL, tail);

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t *)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
e1000_input(struct netif *netif)
{
  struct ifstate *state = netif->state;
  u16_t len = 0;

  uint32_t head = command_read(state, E1000_REG_RXDESCHEAD);
  uint32_t tail = command_read(state, E1000_REG_RXDESCTAIL);
  struct e1000_rx_desc_t *rx_desc = (void*)state->rx_desc;

  uint32_t vtail = tail;
  bool drop = false;
  do {
    vtail = (vtail + 1) % E1000_CONF_RX_RDNUM;

    if ((vtail + 1) % E1000_CONF_TX_TDNUM == head ||
        !(rx_desc[vtail].status & E1000_TXD_STAT_DD))
      break;

    len += rx_desc[vtail].length;
    if (rx_desc[vtail].errors) {
      rx_desc[vtail].errors = 0;
      drop = true;
    }

  } while (!(drop || rx_desc[vtail].status & E1000_TXD_CMD_EOP));

  struct pbuf *p = NULL;
  if (!drop && len >= 60) {
    drop = true;
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  }

  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    struct pbuf *q = p;
    uint16_t q_start = 0, b_start = 0;
    do {
      uint16_t len = q->len - q_start <= rx_desc[tail].length - b_start ?
        q->len - q_start : rx_desc[tail].length - b_start;
      memmove(q->payload + q_start, (void*)rx_desc[tail].address + b_start, len);
      q_start += len;
      b_start += len;

      if (q_start >= q->len) {
        q_start = 0;
        q = q->next;
      }

      if (b_start >= rx_desc[tail].length) {
        b_start = 0;
        tail = (tail + 1) % E1000_CONF_RX_RDNUM;
      }
    } while (q != NULL);

    command_write(state, E1000_REG_RXDESCTAIL, vtail);

    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    if (((u8_t *)p->payload)[0] & 1) {
      /* broadcast or multicast packet*/
      MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
      /* unicast packet*/
      MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }
#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);
  } else {
    if (drop) {
      command_write(state, E1000_REG_RXDESCTAIL, vtail);
    }

    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
  }

  return p;
}

#define GB 1000000000

#define E1000_HW(deviceId) {PCI_VENDOR_INTEL, deviceId, e1000_init, e1000_output, e1000_input, GB, {'e', 'X'}}

const struct eth_hw_handler_t e1000_handlers[5] = {
  E1000_HW(0x100e),
  E1000_HW(0x1004),
  E1000_HW(0x100f),
  E1000_HW(0x10ea),
  E1000_HW(0x10d3)
};
