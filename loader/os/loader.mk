include $(ROOT_DIR)loader/shared/qemu.mk
LOADER_TARGET := $(TARGET_DIR)$(LOADER)

OS_ELF := $(TARGET_DIR)$(LOADER)/boot/kernel.elf
GRUB_ELF := $(TARGET_DIR)$(LOADER)/boot/grub/jmp.bin
GRUB_CFG := $(TARGET_DIR)$(LOADER)/boot/grub/grub.cfg
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


GRUB_S := $(LOADER_ROOT_DIR)grub/start.s
GRUB_C := $(LOADER_ROOT_DIR)grub/main.c
GRUB_LD := $(LOADER_ROOT_DIR)grub/loader.ld

$(GRUB_ELF): $(GRUB_LD) $(GRUB_S) $(GRUB_C)
	$(MKDIR_P) $(LOADER_BUILD_DIR)grub
	$(CC) -nostdlib -fms-extensions -Wno-microsoft-anon-tag -m32 -c $(GRUB_C) -o $(LOADER_BUILD_DIR)grub/main.o \
		-I$(ROOT_DIR)include -I$(LOADER_ROOT_DIR)grub -I$(ROOT_DIR)loader/shared
	$(CC) -nostdlib -m32 -c $(GRUB_S) -o $(LOADER_BUILD_DIR)grub/start.o
	$(MKDIR_P) $(dir $@)
	$(LD) -flavor ld -n -T $(GRUB_LD) $(LOADER_BUILD_DIR)grub/main.o $(LOADER_BUILD_DIR)grub/start.o -o $@

$(GRUB_CFG): $(LOADER_ROOT_DIR)grub/grub.cfg
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

$(LOADER_TARGET): $(EFI_ELF) $(OS_ELF) $(GRUB_ELF) $(GRUB_CFG)

LOADER_PACKAGE := $(TARGET_DIR)$(LOADER).iso
$(LOADER_PACKAGE): build/just
	grub-mkrescue -J -o $@ $(TARGET_DIR)$(LOADER) > /dev/null 2>&1

ifeq ($(RUN_GRUB), 1)
$(LOADER_RUN): $(LOADER_PACKAGE)
	$(QEMU) $(QFLAGS) -cdrom $(LOADER_PACKAGE)
else
$(LOADER_RUN): build
	$(QEMU) $(QFLAGS) $(QEFI) $(QFATROOT)$(LOADER_TARGET)
endif
