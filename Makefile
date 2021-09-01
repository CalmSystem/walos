CC := clang
AS := clang
LD := lld
AR := llvm-ar

ARCH ?= x86_64
KERNEL_ABI := elf
ENTRY_NAME ?= entry.wasm

ROOT_DIR ?= $(dir $(firstword $(MAKEFILE_LIST)))
BUILD_DIR ?= $(ROOT_DIR)build/
TARGET_DIR ?= $(ROOT_DIR)target/

CFLAGS := -MMD -MP -Wall -Werror -mno-red-zone -pedantic
LDFLAGS := -flavor ld
ifneq ($(DEBUG), 1)
CFLAGS += -O3 -flto=thin
LDFLAGS += -strip-all
RELEASE_LOCK := $(BUILD_DIR)release.lock
$(shell $(RM) -f $(BUILD_DIR)debug.lock)
else
CFLAGS += -O1 -g -DDEBUG=1
RELEASE_LOCK := $(BUILD_DIR)debug.lock
$(shell $(RM) -f $(BUILD_DIR)release.lock)
endif

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
CP ?= cp -f
MKDIR_P ?= @mkdir -p

default: all

# Loader
LOADER ?= efi-app
LOADER_ROOT_DIR := $(ROOT_DIR)loader/$(LOADER)/
LOADER_BUILD_DIR := $(BUILD_DIR)loader/$(LOADER)/
LOADER_RUN := run/loader/$(LOADER)
-include $(LOADER_ROOT_DIR)pre-loader.mk
ifeq ($(KERNEL_ABI), elf)
CFLAGS += -DANSI_COLOR=1
endif

# Engine
ENGINE ?= wasm3
ENGINE_ROOT_DIR := $(ROOT_DIR)engine/$(ENGINE)/
ENGINE_BUILD_DIR := $(BUILD_DIR)engine/$(ENGINE)/$(KERNEL_ABI)/
ENGINE_LIB := $(ENGINE_BUILD_DIR)engine.a

# Kernel
KERNEL_ROOT_DIR := $(ROOT_DIR)kernel/
KERNEL_BUILD_DIR := $(BUILD_DIR)kernel/$(KERNEL_ABI)/
KERNEL_LIB := $(KERNEL_BUILD_DIR)kernel.a

include $(LOADER_ROOT_DIR)loader.mk
include $(ENGINE_ROOT_DIR)engine.mk
include $(KERNEL_ROOT_DIR)kernel.mk

-include $(LOADER_DEPS) $(KERNEL_DEPS) $(ENGINE_DEPS)

LOADER_SRV_DIR ?= $(LOADER_TARGET)/srv/
LOADER_TARGET ?= $(LOADER)-has-no-target
LOADER_PACKAGE ?= $(LOADER)-has-no-package
LOADER_ENTRY := $(LOADER_SRV_DIR)$(ENTRY_NAME)
ENTRY_EXT := $(suffix $(ENTRY))
ifeq ($(ENTRY_EXT), .wasm)
ENTRY_WASM := $(ENTRY)
else
ifeq ($(ENTRY_EXT),)
ENTRY_WASM := $(ENTRY).wasm
-include $(ENTRY)/entry.mk
else
ENTRY_WASM := $(ENTRY).wasm
-include $(dir $(ENTRY))/entry$(ENTRY_EXT).mk
endif
endif

# Recipes
.PHONY: clean all default build run package $(LOADER_RUN)

$(RELEASE_LOCK): $(ROOT_DIR)Makefile
	$(MKDIR_P) $(dir $@)
	touch $@

$(LOADER_ENTRY): $(ENTRY_WASM) $(LOADER_TARGET)
	$(MKDIR_P) $(dir $@)
	$(CP) $< $@

build: $(LOADER_ENTRY)
	@echo "Target at $(LOADER_TARGET) !"
run: $(LOADER_ENTRY) $(LOADER_RUN)
package: $(LOADER_ENTRY) $(LOADER_PACKAGE)
	@echo "Package at $(LOADER_PACKAGE) !"

all: package

clean:
	$(RM) -rf $(BUILD_DIR) $(TARGET_DIR)
