QEMU ?= qemu-system-$(ARCH)
OVMF ?= /usr/share/ovmf/OVMF.fd

QFLAGS := -m 1G
ifeq ($(RUN_VGA), 1)
ifeq ($(RUN_LOG), 1)
QFLAGS += -serial stdio
endif
else
QFLAGS += -nographic
endif
ifeq ($(DEBUG), 1)
QFLAGS += -s -S
endif

QEFI := -bios $(OVMF)
QFATROOT := -drive format=raw,file=fat:rw:
