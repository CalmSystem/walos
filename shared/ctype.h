#ifndef CTYPE_H_
#define CTYPE_H_

static inline int isspace(int c) {
	return ((c) == ' ') || ((c) == '\f')
		|| ((c) == '\n') || ((c) == '\r')
		|| ((c) == '\t') || ((c) == '\v');
}

#endif /*CTYPE_H_*/
