include $(ROOT_DIR)sample/common.mk
ENTRY_WASM := $(SAMPLE_BUILD_DIR)vga.wasm
TGA_H := $(SAMPLE_BUILD_DIR)vga/wasm.tga.h

$(TGA_H): $(SAMPLE_ROOT_DIR)vga/wasm.tga $(BIN2HEADER)
	$(MKDIR_P) $(dir $@)
	$(BIN2HEADER) $< _binary_wasm_tga > $@

$(ENTRY_WASM): $(SAMPLE_ROOT_DIR)vga/main.c $(TGA_H)
	$(MKDIR_P) $(dir $@)
	$(CC) $(WASM_FLAGS) -I$(dir $(TGA_H)) $< -o $@