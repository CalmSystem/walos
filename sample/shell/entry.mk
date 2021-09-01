include $(ROOT_DIR)sample/common.mk
ENTRY_WASM := $(SAMPLE_BUILD_DIR)shell.wasm

$(ENTRY_WASM): $(SAMPLE_ROOT_DIR)shell/main.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) $^ -o $@
