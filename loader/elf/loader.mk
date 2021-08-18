LOADER_TARGET := $(TARGET_DIR)$(LOADER)
LOADER_ELF := $(LOADER_TARGET)/run

LOADER_SRCS := $(call rwildcard,$(LOADER_ROOT_DIR),*.c)
LOADER_OBJS := $(LOADER_SRCS:$(LOADER_ROOT_DIR)%=$(LOADER_BUILD_DIR)%.o)
LOADER_DEPS := $(LOADER_OBJS:.o=.d)

DL := /usr/lib64/ld-linux-x86-64.so.2
LPATH := /usr/lib/x86_64-linux-gnu

$(LOADER_BUILD_DIR)%.c.o: $(LOADER_ROOT_DIR)%.c $(KERNEL_CFLAGS_LOCK)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -nostdlib -I$(ROOT_DIR)include -c $< -o $@

$(LOADER_ELF): $(LOADER_KLIB) $(LOADER_LD) $(LOADER_OBJS)
	$(MKDIR_P) $(dir $@)
	$(LD) -flavor ld -dynamic-linker $(DL) -L$(LPATH) $(LPATH)/crt*.o -lc \
		$(LOADER_OBJS) $(LOADER_KLIB) -o $@

$(LOADER_TARGET): $(LOADER_ELF)
# TODO: copy modules and entry

$(LOADER_RUN): $(LOADER_TARGET)
	(cd $(LOADER_TARGET); ./run)
