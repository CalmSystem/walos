#include <w/main>

W_FN_(hw, key_read, char, ())
W_FN_HDL_(stdio, getc, char, ()) {
	return hw_key_read();
}
