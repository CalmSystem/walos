CC := clang
LD := lld
ARCH ?= x86_64

BUILD_DIR ?= build
ROOT_DIR ?= root

QEMU ?= qemu-system-$(ARCH)
OVMF ?= /usr/share/ovmf/OVMF.fd
QFLAGS := \
    -m 256M \
	-bios $(OVMF) -net none \
	-drive format=raw,file=fat:rw:$(ROOT_DIR)

CFLAGS := \
	-ffreestanding -fshort-wchar -MMD -mno-red-zone -std=c11 \
	-O2 -Wall -Werror -pedantic

EFI_SRC := loader/efi.c
EFI_INCS := -Iloader/efi
EFI_OUT := $(ROOT_DIR)/efi/boot/bootx64.efi
EFI_CFLAGS := $(CFLAGS) -target $(ARCH)-unknown-windows
EFI_LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main

K_SRCS := $(wildcard kernel/*.c kernel/**/*.c)
K_LDS := kernel/kernel.ld
K_OBJS := $(K_SRCS:%=$(BUILD_DIR)/%.o)
K_DEPS := $(K_OBJS:.o=.d)
K_INCS := -Iloader -Ikernel/clib
K_OUT := $(ROOT_DIR)/kernel.elf
K_CFLAGS := $(CFLAGS) -target $(ARCH)-unknown-elf -O2
K_LDFLAGS := -flavor ld -T $(K_LDS) -static -Bsymbolic -nostdlib

default: all

$(EFI_OUT): $(EFI_SRC)
	$(MKDIR_P) $(BUILD_DIR)/loader
	$(CC) $(EFI_CFLAGS) $(EFI_INCS) -c $< -o $(BUILD_DIR)/loader/efi.o.c
	$(MKDIR_P) $(dir $@)
	$(LD) $(EFI_LDFLAGS) $(BUILD_DIR)/loader/efi.o.c -out:$@

#$(BUILD_DIR)/%.s.o: %.s
#	$(MKDIR_P) $(dir $@)
#	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(K_CFLAGS) $(K_INCS) -c $< -o $@

$(K_OUT): $(K_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) $(K_LDFLAGS) $(K_OBJS) -o $@

-include $(K_DEPS)

MKDIR_P ?= mkdir -p

.PHONY: clean all default just-qemu qemu

all: $(EFI_OUT) $(K_OUT)
	cp -r bin/* $(ROOT_DIR)

qemu/fast: 
	$(QEMU) $(QFLAGS)
qemu: all qemu/fast

gdb-qemu: all
	$(QEMU) $(QFLAGS) -s -S &
	gdb $(EFI_OUT) -ex 'target remote :1234'

clean:
	$(RM) -r $(BUILD_DIR) $(ROOT_DIR)