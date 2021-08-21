LOADER_TARGET := $(TARGET_DIR)$(LOADER)
LOADER_EFI := $(LOADER_ROOT_DIR)efi.c

LOADER_BOOT := $(LOADER_TARGET)/efi/boot/bootx64.efi
LOADER_DEPS := $(LOADER_BUILD_DIR)efi.d

$(LOADER_BOOT): $(LOADER_EFI) $(LOADER_EFI_LINKED)
	$(MKDIR_P) $(LOADER_BUILD_DIR)
	$(CC) $(CFLAGS) -fno-lto -ffreestanding -fshort-wchar -target $(ARCH)-unknown-windows -I$(ROOT_DIR)include -c $< -o $(LOADER_BUILD_DIR)efi.o
	$(MKDIR_P) $(dir $@)
	$(LD) -flavor link -subsystem:efi_application -entry:efi_main $(LOADER_BUILD_DIR)efi.o $(LOADER_EFI_LINKED) -out:$@

$(LOADER_TARGET): $(LOADER_BOOT) $(LOADER_NEEDS)

QEMU ?= qemu-system-$(ARCH)
OVMF ?= /usr/share/ovmf/OVMF.fd

QFLAGS := -m 1G
ifeq ($(RUN_VGA), 1)
ifeq ($(RUN_LOG), 1)
QFLAGS += -serial stdio
endif
else
QFLAGS += -nographic
endif
ifeq ($(DEBUG), 1)
QFLAGS += -s -S
endif
EFI_QFLAGS := $(QFLAGS) -bios $(OVMF)
EFI_QROOT := -drive format=raw,file=fat:rw:

$(LOADER_RUN): $(LOADER_TARGET)
	$(QEMU) $(EFI_QFLAGS) $(EFI_QROOT)$(LOADER_TARGET)
