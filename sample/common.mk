SAMPLE_ROOT_DIR := $(ROOT_DIR)sample/
SAMPLE_BUILD_DIR := $(BUILD_DIR)sample/

WASM_CFLAGS := --no-standard-libraries -ffreestanding -Wall -Werror -Ofast -target wasm32 -I$(ROOT_DIR)include/mod
WASM_FLAGS := $(WASM_CFLAGS) -Wl,--allow-undefined
WASM_LDFLAGS := -flavor wasm --allow-undefined

# MAYBE: bin2wasm
BIN2HEADER := $(SAMPLE_BUILD_DIR)bin2header
$(BIN2HEADER): $(SAMPLE_ROOT_DIR)bin2header.c
	$(MKDIR_P) $(dir $@)
	$(CC) $^ -o $@
