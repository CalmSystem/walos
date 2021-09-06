include $(ROOT_DIR)sample/bin2h/include.mk
ENTRY_WASM := $(BUILD_DIR)sample/exec.wasm
EMBED_H := $(BUILD_DIR)sample/exec/embed.wasm.h

$(EMBED_H): $(ROOT_DIR)sample/exec/embed.c $(BIN2H)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) $< -o $(BUILD_DIR)sample/exec/embed.wasm
	$(BIN2H) $(BUILD_DIR)sample/exec/embed.wasm _binary_embed_wasm > $@

$(ENTRY_WASM): $(ROOT_DIR)sample/exec/main.c $(EMBED_H)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) -I$(dir $(EMBED_H)) $< -o $@
