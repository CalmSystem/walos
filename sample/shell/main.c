#include <w/stdio.h>

static const w_ciovec backspace = {"\b \b", 3};
void _start(void) {
	char input_line[80];
	w_size input_len = 0;

	while (1) {
		char c = stdio_getc();
		if (c == '\n' || c == '\r' || input_len >= sizeof(input_line)) {
			stdio_putc('\n');
			break;
		}

		if (c == 127) {
			--input_len;
			stdio_write(&backspace, 1);
			continue;
		} else {
			input_line[input_len++] = c;
		}
		stdio_putc(c);
	}
	input_line[input_len] = '\0';

	struct w_ciovec ov = {input_line, input_len};
	stdio_write(&ov, 1);
	stdio_putc('\n');
}
