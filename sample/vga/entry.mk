ENTRY_WASM := $(BUILD_DIR)sample/vga.wasm
TGA_H := $(BUILD_DIR)sample/vga/wasm.tga.h

BIN2HEADER := $(BUILD_DIR)sample/vga/bin2header
$(BIN2HEADER): $(ROOT_DIR)sample/vga/bin2header.c
	$(MKDIR_P) $(dir $@)
	$(CC) $^ -o $@

$(TGA_H): $(ROOT_DIR)sample/vga/wasm.tga $(BIN2HEADER)
	$(MKDIR_P) $(dir $@)
	$(BIN2HEADER) $< _binary_wasm_tga > $@

$(ENTRY_WASM): $(ROOT_DIR)sample/vga/main.c $(TGA_H)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) -I$(dir $(TGA_H)) $< -o $@
