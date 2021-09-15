include $(ROOT_DIR)loader/shared/qemu.mk
LOADER_TARGET := $(TARGET_DIR)$(LOADER)

OS_ELF := $(TARGET_DIR)$(LOADER)/boot/kernel.elf
GRUB_CFG := $(TARGET_DIR)$(LOADER)/boot/grub/grub.cfg
STIVALE_CFG := $(TARGET_DIR)$(LOADER)/boot/limine.cfg
EFI_ELF := $(LOADER_TARGET)/efi/boot/bootx64.efi
EFI_C := $(LOADER_ROOT_DIR)efi.c

OS_SRCS := $(filter-out $(EFI_C),$(wildcard $(LOADER_ROOT_DIR)*.c))
OS_OBJS := $(OS_SRCS:$(LOADER_ROOT_DIR)%=$(LOADER_BUILD_DIR)%.o)
OS_LD := $(LOADER_ROOT_DIR)kernel.ld

$(LOADER_BUILD_DIR)%.c.o: $(LOADER_ROOT_DIR)%.c $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -I$(ROOT_DIR)loader/shared -c $< -o $@

$(OS_ELF): $(KERNEL_LIB) $(ENGINE_LIB) $(OS_LD) $(OS_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) $(KERNEL_LDFLAGS) -static -Bsymbolic -nostdlib -z max-page-size=0x1000 -T $(OS_LD) $(OS_OBJS) $(KERNEL_LIB) $(ENGINE_LIB) -o $@

$(GRUB_CFG): $(LOADER_ROOT_DIR)grub.cfg
	$(MKDIR_P) $(dir $@)
	$(CP) $^ $@
$(STIVALE_CFG): $(LOADER_ROOT_DIR)limine.cfg
	$(MKDIR_P) $(dir $@)
	$(CP) $^ $@

EFI_O := $(LOADER_BUILD_DIR)efi.o
TINF_C := $(ROOT_DIR)loader/shared/uzlib/tinf.c
EFIZ_O := $(LOADER_BUILD_DIR)efiz.o

$(EFI_O): $(EFI_C) $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -fno-lto -ffreestanding -fshort-wchar -target $(ARCH)-unknown-windows -I$(ROOT_DIR)include -I$(ROOT_DIR)loader/shared -c $< -o $@
$(EFIZ_O): $(TINF_C) $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -fno-lto -ffreestanding -fshort-wchar -target $(ARCH)-unknown-windows -c $< -o $@
$(EFI_ELF): $(EFI_O) $(EFIZ_O) $(KERNEL_LIB) $(ENGINE_LIB)
	$(MKDIR_P) $(dir $@)
	$(LD) -flavor link -subsystem:efi_application -entry:efi_main $^ -out:$@


LOADER_DEPS := $(EFI_O:%.o=%.d) $(OS_OBJS:.o=.d)

$(LOADER_TARGET): $(EFI_ELF) $(OS_ELF) $(STIVALE_CFG) $(GRUB_CFG)

LOADER_PACKAGE := $(TARGET_DIR)$(LOADER).iso
LIMINE := $(CACHE_DIR)limine
$(LIMINE):
	git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1 $@
	make -C $@
$(LOADER_PACKAGE): build/just $(LIMINE)
	$(CP) $(LIMINE)/limine.sys $(LIMINE)/limine-cd.bin $(LIMINE)/limine-eltorito-efi.bin $(LOADER_TARGET)/boot
	xorriso -as mkisofs -b boot/limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(LOADER_TARGET) -o $@
	$(LIMINE)/limine-install $@

ifeq ($(RUN_ISO), 1)
$(LOADER_RUN): $(LOADER_PACKAGE)
	$(QEMU) $(QFLAGS) -cdrom $(LOADER_PACKAGE)
else
$(LOADER_RUN): build
	$(QEMU) $(QFLAGS) $(QEFI) $(QFATROOT)$(LOADER_TARGET)
endif
