#include <stdio.h>
#include <stdint.h>

static void help() {
	puts("usage: bin2header <filename> <array-name> [type-name]");
}

const size_t LINE_SIZE = 16;
int main(int argc, char **argv) {

	if (argc < 3 || argc > 4) {
		help();
		return -1;
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		puts("Cannot open file");
		help();
		return -1;
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	rewind(f);

	printf("%s %s[%zu] = {", argc > 3 ? argv[3] : "uint8_t", argv[2], size);

	for (size_t i = 0; i < size; i++) {
		if ((i % LINE_SIZE) == 0) printf("\n\t");

		printf("0x%02x", getc(f));

		if (i != size - 1) printf(",");
	}

	printf("\n};\n");

	return 0;

}
