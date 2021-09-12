struct tar_file_t {
	char* name;
	void* data;
	unsigned long long len;
};

static inline int oct2bin(unsigned char *str, int size) {
	int n = 0;
	unsigned char *c = str;
	while (size-- > 0) {
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}
static inline void* tar_read(void* ptr, struct tar_file_t* out) {
	unsigned char* p = ptr;
	if (memcmp(p + 257, "ustar", 5) != 0) return NULL;

	const int SEC = 512;
	int filesize = oct2bin(p + 124, 11);
	if (out) {
		out->name = (char*)ptr;
		out->data = p + SEC;
		out->len = filesize;
	}
	return p + (((filesize + SEC - 1) / SEC) + 1) * SEC;
}
