#ifndef __XXD_H__
#define __XXD_H__

#include <stdio.h>

/** Minimal human readable hexdump */
static inline void xxd(unsigned int offset, const void *addr, unsigned int len) {
	unsigned int i;
	unsigned char bufferLine[17];
	unsigned char *pc = (unsigned char *)addr;

	for (i = 0; i < len; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				printf(" %s\n", bufferLine);
			printf("%08x: ", offset);
			offset += (i % 16 == 0) ? 16 : i % 16;
		}

		printf("%02x", pc[i]);
		if ((i % 2) == 1)
			printf(" ");

		if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
			bufferLine[i % 16] = '.';
		} else {
			bufferLine[i % 16] = pc[i];
		}

		bufferLine[(i % 16) + 1] = '\0';
	}

	while ((i % 16) != 0) {
		printf("  ");
		if (i % 2 == 1)
			printf(" ");
		i++;
	}
	printf(" %s\n", bufferLine);
}

#endif
