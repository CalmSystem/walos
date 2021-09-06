include $(ROOT_DIR)sample/bin2h/include.mk
ENTRY_WASM := $(BUILD_DIR)sample/vga.wasm
TGA_H := $(BUILD_DIR)sample/vga/wasm.tga.h

$(TGA_H): $(ROOT_DIR)sample/vga/wasm.tga $(BIN2H)
	$(MKDIR_P) $(dir $@)
	$(BIN2H) $< _binary_wasm_tga > $@

$(ENTRY_WASM): $(ROOT_DIR)sample/vga/main.c $(TGA_H)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) -I$(dir $(TGA_H)) $< -o $@
