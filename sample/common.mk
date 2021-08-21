SAMPLE_ROOT_DIR := $(ROOT_DIR)sample/
SAMPLE_BUILD_DIR := $(BUILD_DIR)sample/

WASM_CFLAGS := --no-standard-libraries -ffreestanding -Wall -Werror -Ofast -target wasm32 -I$(ROOT_DIR)/include/mod -Wl,--allow-undefined
