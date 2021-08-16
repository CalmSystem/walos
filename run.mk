# included from Makefile

IMG_OUT ?= walos.img
ISO_OUT ?= walos.iso

QEMU ?= qemu-system-$(ARCH)
OVMF ?= /usr/share/ovmf/OVMF.fd
QFLAGS := -m 1G -bios $(OVMF)
QROOT := -drive format=raw,file=fat:rw:$(ROOT_DIR)
# Old syntax was -usb -usbdevice disk::$(IMG_OUT)
QIMG := -drive format=raw,if=none,id=stick,file=$(IMG_OUT) \
	-device nec-usb-xhci,id=xhci \
	-device usb-storage,bus=xhci.0,drive=stick
QFLOPPY := $(QROOT) -fda $(IMG_OUT)
QISO := -cdrom $(ISO_OUT)

img: root
	dd if=/dev/zero of=$(IMG_OUT) bs=1k count=1440
	mformat -i $(IMG_OUT) -f 1440 -v "walos" ::
	mcopy -i $(IMG_OUT) -s $(ROOT_DIR)/* ::

iso: img
	$(MKDIR_P) $(BUILD_DIR)/iso
	cp $(IMG_OUT) $(BUILD_DIR)/iso
	xorriso -as mkisofs -R -f -e $(IMG_OUT) -no-emul-boot -o $(ISO_OUT) $(BUILD_DIR)/iso
	rm -r $(BUILD_DIR)/iso

qemu/fast:
	$(QEMU) $(QFLAGS) $(QROOT)
qemu: root qemu/fast

qemu/floppy/fast:
	$(QEMU) $(QFLAGS) $(QFLOPPY)
qemu/floppy: img qemu/floppy/fast

qemu/usb/fast:
	$(QEMU) $(QFLAGS) $(QIMG)
qemu/usb: img qemu/usb/fast

qemu/iso/fast:
	$(QEMU) $(QFLAGS) $(QISO)
qemu/iso: iso qemu/iso/fast

qemu/debug/fast: QFLAGS += -s -S
qemu/debug/fast: qemu/fast

qemu/debug: root/debug qemu/debug/fast

gdb-qemu: root/debug
	$(QEMU) $(QFLAGS) -s -S &
	gdb $(K_OUT) -ex 'target remote :1234'

clean/run:
	$(RM) -rf $(IMG_OUT) $(ISO_OUT)
