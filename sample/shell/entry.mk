ENTRY_WASM := $(BUILD_DIR)sample/shell.wasm

$(ENTRY_WASM): $(ROOT_DIR)sample/shell/main.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) $^ -o $@
