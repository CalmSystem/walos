CC := clang
LD := lld
ARCH ?= x86-64

BUILD_DIR ?= build
ROOT_DIR ?= root

include arch/$(ARCH).env

export

CFLAGS := \
	-ffreestanding -fshort-wchar -MMD -mno-red-zone -std=c11 \
	-target $(CARCH) -O2 -Wall -Werror -pedantic
LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main
QFLAGS := \
	-bios $(OVMF) -net none \
	-drive format=raw,file=fat:rw:$(ROOT_DIR)

BOOT_SRCS := $(wildcard boot/*.c shared/*.c)
BOOT_OBJS := $(BOOT_SRCS:%=$(BUILD_DIR)/%.o)
BOOT_DEPS := $(BOOT_OBJS:.o=.d)
BOOT_INCS := -Iboot/efi -Ishared
BOOT_EFI := $(ROOT_DIR)/efi/boot/bootx64.efi

default: all

#$(BUILD_DIR)/%.s.o: %.s
#	$(MKDIR_P) $(dir $@)
#	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(BOOT_INCS) -c $< -o $@

$(BOOT_EFI): $(BOOT_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) $(LDFLAGS) $(BOOT_OBJS) -out:$@

-include $(BOOT_DEPS)

MKDIR_P ?= mkdir -p

.PHONY: clean all default just-qemu qemu

all: $(BOOT_EFI)

qemu/fast: 
	$(QEMU) $(QFLAGS)
qemu: all qemu/fast

gdb-qemu: all
	$(QEMU) $(QFLAGS) -s -S &
	gdb $(BOOT_EFI) -ex 'target remote :1234'

clean:
	$(RM) -r $(BUILD_DIR) $(ROOT_DIR)