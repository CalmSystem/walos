KERNEL_SRCS := $(call rwildcard,$(KERNEL_ROOT_DIR),*.c)
KERNEL_OBJS := $(KERNEL_SRCS:$(KERNEL_ROOT_DIR)%=$(KERNEL_BUILD_DIR)%.o)
KERNEL_DEPS := $(KERNEL_OBJS:.o=.d)
KERNEL_CFLAGS := $(CFLAGS) -ffreestanding -target $(ARCH)-unknown-$(KERNEL_ABI) -I$(ROOT_DIR)include
KERNEL_LDFLAGS := $(LDFLAGS)

$(KERNEL_BUILD_DIR)%.c.o: $(KERNEL_ROOT_DIR)%.c $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(KERNEL_LIB): $(KERNEL_OBJS)
	$(MKDIR_P) $(dir $@)
	$(AR) rsc $@ $^
