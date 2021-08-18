CC := clang
AS := clang
LD := lld
AR := llvm-ar

ARCH ?= x86_64
KERNEL_ABI := elf

ROOT_DIR ?= $(dir $(firstword $(MAKEFILE_LIST)))
BUILD_DIR ?= $(ROOT_DIR)build/
TARGET_DIR ?= $(ROOT_DIR)target/

CFLAGS := -MMD -MP -Wall -Werror -mno-red-zone # -pedantic

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
CP ?= cp -f
MKDIR_P ?= @mkdir -p

default: all

# Loader
LOADER ?= efi-app
LOADER_ROOT_DIR := $(ROOT_DIR)loader/$(LOADER)/
LOADER_BUILD_DIR := $(BUILD_DIR)loader/$(LOADER)/
LOADER_KLIB := $(LOADER_BUILD_DIR)kernel.a
LOADER_RUN := run/loader/$(LOADER)
include $(LOADER_ROOT_DIR)loader.mk

LOADER_TARGET ?= $(LOADER)-has-no-target
LOADER_PACKAGE ?= $(LOADER)-has-no-package

# Kernel
KERNEL_ROOT_DIR := $(ROOT_DIR)kernel/
KERNEL_BUILD_DIR := $(BUILD_DIR)kernel/$(KERNEL_ABI)/
KERNEL_LIB := $(KERNEL_BUILD_DIR)kernel.a
include $(KERNEL_ROOT_DIR)kernel.mk

-include $(LOADER_DEPS) $(KERNEL_DEPS)

# Recipes
.PHONY: clean all default build run package $(LOADER_RUN)

$(LOADER_KLIB): $(KERNEL_LIB)
	$(MKDIR_P) $(dir $@)
	$(CP) $^ $@

build: $(LOADER_TARGET)
	@echo "Target at $(LOADER_TARGET) !"
run: $(LOADER_RUN)
package: $(LOADER_PACKAGE)
	@echo "Package at $(LOADER_PACKAGE) !"

all: package

clean:
	$(RM) -rf $(BUILD_DIR) $(TARGET_DIR)
