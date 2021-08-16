OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS {
	. = 0x100000;
	_kernel_start = .;
	.text : ALIGN(4K) {
		KEEP(*(.multiboot))
		*(.text)
	}
	.rodata : ALIGN(4K) {
		*(.rodata)
	}
	.data : ALIGN(4K) {
		*(.data)
	}
	.bss : ALIGN(4K) {
		*(COMMON)
		*(.bss)
	}
	_kernel_end = .;

	/* Direct memory access */
	floppy_dmabuf = 0x8000;
	floppy_dmalen = 0x4800;

	/DISCARD/ : {
		*(.comment)
	}
	_heap_start = 0x1000000;
}
