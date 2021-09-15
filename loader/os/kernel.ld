OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS {
	. = 0x100000;
	_kernel_start = .;
	.text : ALIGN(4K) {
		*(.text)
	}
	.stivalehdr : ALIGN(4K) {
		KEEP(*(.stivalehdr))
	}
	.rodata : {
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

	/DISCARD/ : {
		*(.comment)
	}
	_heap_start = 0x1000000;
}
