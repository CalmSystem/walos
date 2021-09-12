include $(ROOT_DIR)loader/shared/qemu.mk
LOADER_TARGET := $(TARGET_DIR)$(LOADER)

LOADER_C := $(LOADER_ROOT_DIR)efi.c
LOADER_O := $(LOADER_BUILD_DIR)efi.o
LOADER_DEPS := $(LOADER_BUILD_DIR)efi.d

LOADER_BOOT := $(LOADER_TARGET)/efi/boot/bootx64.efi

$(LOADER_O): $(LOADER_C) $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -fno-lto -ffreestanding -fshort-wchar -target $(ARCH)-unknown-windows -I$(ROOT_DIR)include -I$(ROOT_DIR)loader/shared -c $< -o $@
$(LOADER_BOOT): $(LOADER_O) $(KERNEL_LIB) $(ENGINE_LIB)
	$(MKDIR_P) $(dir $@)
	$(LD) -flavor link -subsystem:efi_application -entry:efi_main $^ -out:$@

$(LOADER_TARGET): $(LOADER_BOOT) $(LOADER_NEEDS)
$(LOADER_RUN): build
	$(MKDIR_P) $(LOADER_TARGET)/entry
	tar -xf $(LOADER_ENTRY) -C $(LOADER_TARGET)/entry
	$(QEMU) $(QFLAGS) $(QEFI) $(QFATROOT)$(LOADER_TARGET)
