include $(ROOT_DIR)sample/common.mk
ENTRY_WASM := $(SAMPLE_BUILD_DIR)$(ENTRY).wasm

$(ENTRY_WASM): $(ENTRY)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) $^ -o $@
