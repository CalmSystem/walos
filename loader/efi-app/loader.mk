KERNEL_ABI := windows
LOADER_EFI_LINKED := $(LOADER_KLIB) 
include $(ROOT_DIR)/loader/efi-common.mk
