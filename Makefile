CC := clang
AS := clang
LD := lld
ARCH ?= x86_64

WASI_SDK_VERSION ?= 12
WASI_SDK_REVISION ?= 0
WASI_SDK ?= $(HOME)/.cache/wasi-sdk-$(WASI_SDK_VERSION)
WASI_CC := $(WASI_SDK)/bin/clang --sysroot=$(WASI_SDK)/share/wasi-sysroot

BUILD_DIR ?= build
ROOT_DIR ?= root
IMG_OUT ?= walos.img
ISO_OUT ?= walos.iso

QEMU ?= qemu-system-$(ARCH)
OVMF ?= /usr/share/ovmf/OVMF.fd
QFLAGS := -m 1G -bios $(OVMF)
QROOT := -drive format=raw,file=fat:rw:$(ROOT_DIR)
# Old syntax was -usb -usbdevice disk::$(IMG_OUT)
QIMG := -drive format=raw,if=none,id=stick,file=$(IMG_OUT) \
	-device nec-usb-xhci,id=xhci \
	-device usb-storage,bus=xhci.0,drive=stick
QISO := -cdrom $(ISO_OUT)

CFLAGS := \
	-ffreestanding -fshort-wchar -MMD -mno-red-zone -std=c11 \
	-Wall -Werror # -pedantic

EFI_SRC := loader/efi.c
EFI_INCS := -Iloader/efi -Iinclude
EFI_OUT := $(ROOT_DIR)/efi/boot/bootx64.efi
EFI_CFLAGS := $(CFLAGS) -target $(ARCH)-unknown-windows -O2
EFI_LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
K_SRCS := kernel/handlers.S $(call rwildcard,kernel/,*.c)
K_LDS := kernel/kernel.ld
K_OBJS := $(K_SRCS:%=$(BUILD_DIR)/%.o)
K_DEPS := $(K_OBJS:.o=.d)
K_INCS := -Iinclude -Ikernel/libc -Ikernel/net/include
K_OUT := $(ROOT_DIR)/boot/kernel.elf
K_CFLAGS := $(CFLAGS) -target $(ARCH)-unknown-elf -O3 -flto=thin
K_LDFLAGS := -flavor ld -T $(K_LDS) -static -Bsymbolic -nostdlib -z max-page-size=0x1000 -strip-all

WASM_RAW_FLAGS := --no-standard-libraries -ffreestanding -Wall -Werror -Ofast -target wasm32 -Isrv/libc
WASM_WASI_FLAGS := -Wall -Werror -Ofast
SRV_FLAGS := -Isrv/include -Wl,--export="srv_prehandle" -Wl,--export="srv_handle" -Wl,--allow-undefined-file="srv/include/extern.syms"
SRV_OUTS := \
	$(patsubst %.c,$(ROOT_DIR)/%.wasm,$(wildcard srv/*.c)) \
	$(patsubst %.cpp,$(ROOT_DIR)/%.wasm,$(wildcard srv/*.cpp))
SRV_OUTS := $(patsubst %.wasi.wasm,%.wasm,$(SRV_OUTS))

default: all

$(EFI_OUT): $(EFI_SRC)
	$(MKDIR_P) $(BUILD_DIR)/loader
	$(CC) $(EFI_CFLAGS) $(EFI_INCS) -c $< -o $(BUILD_DIR)/loader/efi.o.c
	$(MKDIR_P) $(dir $@)
	$(LD) $(EFI_LDFLAGS) $(BUILD_DIR)/loader/efi.o.c -out:$@

$(BUILD_DIR)/%.S.o: %.S
	$(MKDIR_P) $(dir $@)
	$(AS) $(K_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(K_CFLAGS) $(K_INCS) -c $< -o $@

$(K_OUT): $(K_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) $(K_LDFLAGS) $^ -o $@

$(ROOT_DIR)/srv/%.wasm: srv/%.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_RAW_FLAGS) $(SRV_FLAGS) $< -o $@

$(ROOT_DIR)/srv/%.wasm: srv/%.cpp
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_RAW_FLAGS) $(SRV_FLAGS) $< -o $@

$(WASI_SDK):
	echo "Downloading WASI SDK"
	wget -nv --show-progress -O $(BUILD_DIR)/wasi.tar.gz \
		https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-$(WASI_SDK_VERSION)/wasi-sdk-$(WASI_SDK_VERSION).$(WASI_SDK_REVISION)-linux.tar.gz
	tar xf $(BUILD_DIR)/wasi.tar.gz --checkpoint=.100 -C $(BUILD_DIR)
	rm $(BUILD_DIR)/wasi.tar.gz
	mv $(BUILD_DIR)/wasi-sdk-$(WASI_SDK_VERSION).$(WASI_SDK_REVISION) $(WASI_SDK)

$(ROOT_DIR)/srv/%.wasm: srv/%.wasi.c $(WASI_SDK)
	$(MKDIR_P) $(dir $@)
	$(WASI_CC) $(WASM_WASI_FLAGS) $(SRV_FLAGS) $< -o $@

$(ROOT_DIR)/srv/%.wasm: srv/%.wasi.cpp $(WASI_SDK)
	$(MKDIR_P) $(dir $@)
	$(WASI_CC) $(WASM_WASI_FLAGS) $(SRV_FLAGS) $< -o $@

-include $(K_DEPS)

MKDIR_P ?= @mkdir -p

.PHONY: clean all default qemu

root: bin $(EFI_OUT) $(K_OUT) $(SRV_OUTS)
	cp -r bin/* $(ROOT_DIR)

root/debug: K_CFLAGS := $(filter-out -O3 -flto=thin,$(K_CFLAGS))
root/debug: K_CFLAGS += -g -ggdb -O1
root/debug: K_LDFLAGS := $(filter-out -strip-all,$(K_LDFLAGS))
root/debug: root

img: root
	dd if=/dev/zero of=$(IMG_OUT) bs=1k count=1440
	mformat -i $(IMG_OUT) -f 1440 ::
	mcopy -i $(IMG_OUT) -s $(ROOT_DIR)/* ::

iso: img
	$(MKDIR_P) $(BUILD_DIR)/iso
	cp $(IMG_OUT) $(BUILD_DIR)/iso
	xorriso -as mkisofs -R -f -e $(IMG_OUT) -no-emul-boot -o $(ISO_OUT) $(BUILD_DIR)/iso
	rm -r $(BUILD_DIR)/iso

qemu/fast:
	$(QEMU) $(QFLAGS) $(QROOT)
qemu: root qemu/fast

qemu/usb/fast:
	$(QEMU) $(QFLAGS) $(QIMG)
qemu/usb: img qemu/usb/fast

qemu/iso/fast:
	$(QEMU) $(QFLAGS) $(QISO)
qemu/iso: iso qemu/iso/fast

qemu/debug/fast: QFLAGS += -s -S
qemu/debug/fast: qemu/fast

qemu/debug: root/debug qemu/debug/fast

gdb-qemu: root/debug
	$(QEMU) $(QFLAGS) -s -S &
	gdb $(K_OUT) -ex 'target remote :1234'

all: root iso

clean:
	$(RM) -rf $(BUILD_DIR) $(ROOT_DIR) $(IMG_OUT) $(ISO_OUT)