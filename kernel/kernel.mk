KERNEL_SRCS := $(call rwildcard,$(KERNEL_ROOT_DIR),*.c)
KERNEL_OBJS := $(KERNEL_SRCS:$(KERNEL_ROOT_DIR)%=$(KERNEL_BUILD_DIR)%.o)
KERNEL_DEPS := $(KERNEL_OBJS:.o=.d)
KERNEL_CFLAGS := $(CFLAGS) -ffreestanding -target $(ARCH)-unknown-$(KERNEL_ABI) -I$(ROOT_DIR)include
KERNEL_LDFLAGS := -flavor ld -static -Bsymbolic -nostdlib
ifneq ($(DEBUG_GDB), 1)
KERNEL_CFLAGS += -O3 -flto=thin
KERNEL_LDFLAGS += -strip-all
KERNEL_CFLAGS_LOCK := $(KERNEL_BUILD_DIR)release.lock
$(shell $(RM) -f $(KERNEL_BUILD_DIR)debug.lock)
else
KERNEL_CFLAGS += -O1 -g -ggdb
KERNEL_CFLAGS_LOCK := $(KERNEL_BUILD_DIR)debug.lock
$(shell $(RM) -f $(KERNEL_BUILD_DIR)release.lock)
endif

$(KERNEL_CFLAGS_LOCK):
	$(MKDIR_P) $(dir $@)
	touch $@

$(KERNEL_BUILD_DIR)%.c.o: $(KERNEL_ROOT_DIR)%.c $(KERNEL_CFLAGS_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(KERNEL_LIB): $(KERNEL_OBJS)
	$(MKDIR_P) $(dir $@)
	$(AR) rsc $@ $^
