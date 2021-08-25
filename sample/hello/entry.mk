include $(ROOT_DIR)sample/common.mk
ENTRY_WASM := $(SAMPLE_BUILD_DIR)hello.wasm

$(ENTRY_WASM): $(SAMPLE_ROOT_DIR)hello/main.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) $^ -o $@
