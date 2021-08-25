LOADER_ELF := $(TARGET_DIR)$(LOADER)/boot/kernel.elf
LOADER_NEEDS += $(LOADER_ELF)
include $(ROOT_DIR)loader/shared/efi.mk

LOADER_SRCS := $(filter-out $(LOADER_EFI),$(call rwildcard,$(LOADER_ROOT_DIR),*.c))
LOADER_OBJS := $(LOADER_SRCS:$(LOADER_ROOT_DIR)%=$(LOADER_BUILD_DIR)%.o)
LOADER_DEPS += $(LOADER_OBJS:.o=.d)

LOADER_LD := $(LOADER_ROOT_DIR)kernel.ld

$(LOADER_BUILD_DIR)%.c.o: $(LOADER_ROOT_DIR)%.c $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -I$(ROOT_DIR)loader/shared -c $< -o $@

$(LOADER_ELF): $(KERNEL_LIB) $(ENGINE_LIB) $(LOADER_LD) $(LOADER_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) $(KERNEL_LDFLAGS) -static -Bsymbolic -nostdlib -z max-page-size=0x1000 -T $(LOADER_LD) $(LOADER_OBJS) $(KERNEL_LIB) $(ENGINE_LIB) -o $@
