WASM_LDARGS := --allow-undefined --export-dynamic -O3 --gc-sections --strip-all
WASM_CFLAGS := -target wasm32 --no-standard-libraries -ffreestanding -Wall -Werror -Os -flto=thin -I$(ROOT_DIR)include
WASM_FLAGS := $(WASM_CFLAGS) $(WASM_LDARGS:%=-Wl,%)
WASM_LDFLAGS := -flavor wasm $(WASM_LDARGS)

SERVICE_SIMPLE_SRC := $(wildcard $(SERVICE_ROOT_DIR)*.c)
SERVICE_SIMPLE_WASM := $(SERVICE_SIMPLE_SRC:$(SERVICE_ROOT_DIR)%.c=$(SERVICE_BUILD_DIR)%.wasm)

SERVICE_COMPLEX_SRC := $(wildcard $(SERVICE_ROOT_DIR)*/service.mk)
SERVICE_COMPLEX_WASM := $(SERVICE_COMPLEX_SRC:$(SERVICE_ROOT_DIR)%/service.mk=$(SERVICE_BUILD_DIR)%.wasm)
include $(SERVICE_COMPLEX_SRC)

SERVICE_OUT := $(SERVICE_SIMPLE_WASM) $(SERVICE_COMPLEX_WASM)

$(SERVICE_BUILD_DIR)%.wasm: $(SERVICE_ROOT_DIR)%.c
	$(MKDIR_P) $(dir $@)
ifeq ($(CUSTOM_LINK), 1)
	$(error Custom link section unimplemented)
else
	$(CC) $(WASM_FLAGS) -Wl,--no-entry -DW_LOW_ENTRY=_initialize $^ -o $@
endif
