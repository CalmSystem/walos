ENGINE_SRCS := $(call rwildcard,$(ENGINE_ROOT_DIR),*.c)
ENGINE_OBJS := $(ENGINE_SRCS:$(ENGINE_ROOT_DIR)%=$(ENGINE_BUILD_DIR)%.o)
ENGINE_DEPS := $(ENGINE_OBJS:.o=.d)

$(ENGINE_BUILD_DIR)%.c.o: $(ENGINE_ROOT_DIR)%.c $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(ENGINE_LIB): $(ENGINE_OBJS)
	$(MKDIR_P) $(dir $@)
	$(AR) rsc $@ $^
