LOADER_ELF := $(TARGET_DIR)$(LOADER)/boot/kernel.elf
LOADER_NEEDS += $(LOADER_ELF)
include $(ROOT_DIR)/loader/efi-common.mk

LOADER_SRCS := $(filter-out $(LOADER_EFI),$(call rwildcard,$(LOADER_ROOT_DIR),*.c))
LOADER_OBJS := $(LOADER_SRCS:$(LOADER_ROOT_DIR)%=$(LOADER_BUILD_DIR)%.o)
LOADER_DEPS += $(LOADER_OBJS:.o=.d)

LOADER_LD := $(LOADER_ROOT_DIR)kernel.ld

$(LOADER_BUILD_DIR)%.c.o: $(LOADER_ROOT_DIR)%.c $(KERNEL_CFLAGS_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(LOADER_ELF): $(LOADER_KLIB) $(LOADER_LD) $(LOADER_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) $(KERNEL_LDFLAGS) -T $(LOADER_LD) $(LOADER_OBJS) $(LOADER_KLIB) -o $@
