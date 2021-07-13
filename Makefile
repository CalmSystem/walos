CC := clang
LD := lld
ARCH ?= x86_64

BUILD_DIR ?= build
ROOT_DIR ?= root

QEMU ?= qemu-system-$(ARCH)
OVMF ?= /usr/share/ovmf/OVMF.fd
QFLAGS := \
    -m 1G \
	-bios $(OVMF) -net none \
	-drive format=raw,file=fat:rw:$(ROOT_DIR)

CFLAGS := \
	-ffreestanding -fshort-wchar -MMD -mno-red-zone -std=c11 \
	-O2 -Wall -Werror # -pedantic

EFI_SRC := loader/efi.c
EFI_INCS := -Iloader/efi -Iinclude
EFI_OUT := $(ROOT_DIR)/efi/boot/bootx64.efi
EFI_CFLAGS := $(CFLAGS) -target $(ARCH)-unknown-windows
EFI_LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main

K_SRCS := $(wildcard kernel/*.c kernel/**/*.c)
K_LDS := kernel/kernel.ld
K_OBJS := $(K_SRCS:%=$(BUILD_DIR)/%.o)
K_DEPS := $(K_OBJS:.o=.d)
K_INCS := -Iinclude -Ikernel/libc
K_OUT := $(ROOT_DIR)/kernel.elf
K_CFLAGS := $(CFLAGS) -target $(ARCH)-unknown-elf -O2
K_LDFLAGS := -flavor ld -T $(K_LDS) -static -Bsymbolic -nostdlib -strip-all

WASM_FLAGS := --no-standard-libraries -ffreestanding -Wall -Werror -pedantic -Ofast -target wasm32
SRV_FLAGS := $(WASM_FLAGS) -Isrv/include -Wl,--export="handle" -Wl,--allow-undefined-file="srv/include/extern.syms"
SRV_OUTS := \
	$(patsubst %.c,$(ROOT_DIR)/%.wasm,$(wildcard srv/*.c)) \
	$(patsubst %.cpp,$(ROOT_DIR)/%.wasm,$(wildcard srv/*.cpp))

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
	$(LD) $(K_LDFLAGS) $^ -o $@

$(ROOT_DIR)/srv/%.wasm: srv/%.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(SRV_FLAGS) $< -o $@

$(ROOT_DIR)/srv/%.wasm: srv/%.cpp
	$(MKDIR_P) $(dir $@)
	$(CC) $(SRV_FLAGS) $< -o $@

-include $(K_DEPS)

MKDIR_P ?= mkdir -p

.PHONY: clean all default just-qemu qemu

root: $(EFI_OUT) $(K_OUT) $(SRV_OUTS)
	cp -r bin/* $(ROOT_DIR)

qemu/fast:
	$(QEMU) $(QFLAGS)
qemu: root qemu/fast

gdb-qemu: root
	$(QEMU) $(QFLAGS) -s -S &
	gdb $(EFI_OUT) -ex 'target remote :1234'

all: root

clean:
	$(RM) -rf $(BUILD_DIR) $(ROOT_DIR)