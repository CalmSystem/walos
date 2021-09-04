ENTRY_WASM := $(BUILD_DIR)$(ENTRY).wasm

$(ENTRY_WASM): $(ENTRY)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) $^ -o $@
