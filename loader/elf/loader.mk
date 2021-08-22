LOADER_TARGET := $(TARGET_DIR)$(LOADER)
LOADER_ELF := $(LOADER_TARGET)/run

LOADER_SRCS := $(call rwildcard,$(LOADER_ROOT_DIR),*.c)
LOADER_OBJS := $(LOADER_SRCS:$(LOADER_ROOT_DIR)%=$(LOADER_BUILD_DIR)%.o)
LOADER_DEPS := $(LOADER_OBJS:.o=.d)

DLINK ?= /usr/lib64/ld-linux-x86-64.so.2
LDPATH ?= /usr/lib/x86_64-linux-gnu

$(LOADER_BUILD_DIR)%.c.o: $(LOADER_ROOT_DIR)%.c $(RELEASE_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -I$(ROOT_DIR)include -c $< -o $@

$(LOADER_ELF): $(KERNEL_LIB) $(ENGINE_LIB) $(LOADER_LD) $(LOADER_OBJS)
	$(MKDIR_P) $(dir $@)
# Do system gcc linking job manually cause it fails with x86_64-unknown-elf
	$(LD) $(LDFLAGS) -dynamic-linker $(DLINK) -L$(LDPATH) $(LDPATH)/crt*.o -lc $(LOADER_OBJS) $(KERNEL_LIB) $(ENGINE_LIB) -o $@

$(LOADER_TARGET): $(LOADER_ELF)

$(LOADER_RUN): $(LOADER_TARGET)
	(cd $(LOADER_TARGET); ./run)
