/*
#include <w/stdio.h>
#include <w/net.h>
#include <stddef.h>

static w_ciovec url[] = W_IOBUF("http://duckduckgo.com");
void _start() {
	struct http_request_t req = {0};
	req.method = HTTP_GET;
	req.url.iov = url;
	req.url.iocnt = 1;

	uint8_t buffer[512];
	w_iovec body[] = W_IOBUF(buffer);
	struct http_response_t res = {0};
	res.body.iov = body;
	res.body.iocnt = 1;

	http_call(req, &res);
	stdio_putc('0'+(res.status / 100));
	stdio_putc('0'+(res.status / 10 % 10));
	stdio_putc('0'+(res.status % 10));
	stdio_putc(' ');
	if (res.status) {
		w_ciovec bodyc[] = {{buffer, res.body.got_len}};
		stdio_write(bodyc, 1);
	}
}
*/

#include <w/main>

w_res yolo(w_res r) { return (r + 1) * 2; }

W_FN(net, test, w_res, (w_res (*cb)(w_res)), {ST_FN})
W_MAIN() {
	net_test(yolo);
}
