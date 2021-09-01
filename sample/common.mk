SAMPLE_ROOT_DIR := $(ROOT_DIR)sample/
SAMPLE_BUILD_DIR := $(BUILD_DIR)sample/

WASM_LDARGS := --allow-undefined --export-dynamic -O3 --gc-sections --strip-all
WASM_CFLAGS := -target wasm32 --no-standard-libraries -ffreestanding -Wall -Werror -Os -flto=thin -I$(ROOT_DIR)include
WASM_FLAGS := $(WASM_CFLAGS) $(WASM_LDARGS:%=-Wl,%)
WASM_LDFLAGS := -flavor wasm $(WASM_LDARGS)
