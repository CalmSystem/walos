#ifndef __MOD_HTTP_H
#define __MOD_HTTP_H
#include "types.h"

enum http_method {
	HTTP_GET,
	HTTP_POST,
	HTTP_PATCH,
	HTTP_OPTIONS,
	HTTP_CONNECT,
	HTTP_HEAD,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_TRACE
};
typedef uint16_t http_status;
W_FN(http, request, http_status, (enum http_method method, const w_ciovec* req_urlv, w_size req_urlc, \
	const w_ciovec* req_bodyv, w_size req_bodyc, const w_ciovec* req_headerv, w_size req_headersc, \
	w_iovec* res_bodyv, w_size res_bodyc, w_size* res_body_len, w_iovec* res_headersv, w_size res_headersc, w_size* res_headers_len), \
	{ST_VAL, ST_CIO, ST_LEN, ST_CIO, ST_LEN, ST_CIO, ST_LEN, ST_BIO, ST_LEN, ST_PTR, ST_BIO, ST_LEN, ST_PTR})

struct http_buf_req_t {
	const w_ciovec* iov;
	w_size iocnt;
};
struct http_request_t {
	enum http_method method;
	struct http_buf_req_t url;
	struct http_buf_req_t body;
	struct http_buf_req_t header;
};
struct http_buf_resp_t {
	w_iovec* iov;
	w_size iocnt;
	w_size got_len;
};
struct http_response_t {
	http_status status;
	struct http_buf_resp_t body;
	struct http_buf_resp_t headers;
};
void http_call(struct http_request_t req, struct http_response_t* res) {
	res->status = http_request(req.method, req.url.iov, req.url.iocnt, req.body.iov, req.body.iocnt, req.header.iov, req.header.iocnt,
		res->body.iov, res->body.iocnt, &res->body.got_len, res->headers.iov, res->headers.iocnt, &res->headers.got_len);
}

#endif
