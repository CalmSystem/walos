#include "stdlib.h"
#include "limits.h"
#include "ctype.h"
#include "div64.h"

// Extracted from linux kernel vsprintf.c - GPL 2.0

#ifdef __GNUC__
#define unlikely(x) __builtin_expect(x, 0)
#else
#define unlikely(x) (x)
#endif

#define KSTRTOX_OVERFLOW	(1U << 31)

static inline const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
	if (*base == 0) {
		if (s[0] == '0') {
			if (_tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
			else
				*base = 8;
		} else
			*base = 10;
	}
	if (*base == 16 && s[0] == '0' && _tolower(s[1]) == 'x')
		s += 2;
	return s;
}

/*
 * Convert non-negative integer string representation in explicitly given radix
 * to an integer. A maximum of max_chars characters will be converted.
 *
 * Return number of characters consumed maybe or-ed with overflow bit.
 * If overflow occurs, result integer (incorrect) is still returned.
 *
 * Don't you dare use this function.
 */
static inline unsigned int _parse_integer_limit(const char *s, unsigned int base, unsigned long long *p,
				  size_t max_chars)
{
	unsigned long long res;
	unsigned int rv;

	res = 0;
	rv = 0;
	while (max_chars--) {
		unsigned int c = *s;
		unsigned int lc = c | 0x20; /* don't tolower() this line */
		unsigned int val;

		if ('0' <= c && c <= '9')
			val = c - '0';
		else if ('a' <= lc && lc <= 'f')
			val = lc - 'a' + 10;
		else
			break;

		if (val >= base)
			break;
		/*
		 * Check for overflow only if we are within range of
		 * it in the max base we support (16)
		 */
		if (unlikely(res & (~0ull << 60))) {
			if (res > div_u64(ULLONG_MAX - val, base))
				rv |= KSTRTOX_OVERFLOW;
		}
		res = res * base + val;
		rv++;
		s++;
	}
	*p = res;
	return rv;
}

static unsigned long long simple_strntoull(const char *startp, size_t max_chars,
					   char **endp, unsigned int base)
{
	const char *cp;
	unsigned long long result = 0ULL;
	size_t prefix_chars;
	unsigned int rv;

	cp = _parse_integer_fixup_radix(startp, &base);
	prefix_chars = cp - startp;
	if (prefix_chars < max_chars) {
		rv = _parse_integer_limit(cp, base, &result, max_chars - prefix_chars);
		/* FIXME */
		cp += (rv & ~KSTRTOX_OVERFLOW);
	} else {
		/* Field too short for prefix + digit, skip over without converting */
		cp = startp + max_chars;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

/**
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 *
 * This function has caveats. Please use kstrtoull instead.
 */
unsigned long long strtoull(const char *cp, char **endp, int base)
{
	return simple_strntoull(cp, INT_MAX, endp, base);
}

/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 *
 * This function has caveats. Please use kstrtoul instead.
 */
unsigned long strtoul(const char *cp, char **endp, int base)
{
	return strtoull(cp, endp, base);
}

/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 *
 * This function has caveats. Please use kstrtol instead.
 */
long strtol(const char *cp, char **endp, int base)
{
	if (*cp == '-')
		return -strtoul(cp + 1, endp, base);

	return strtoul(cp, endp, base);
}

static long long strntoll(const char *cp, size_t max_chars, char **endp,
				 unsigned int base)
{
	/*
	 * simple_strntoull() safely handles receiving max_chars==0 in the
	 * case cp[0] == '-' && max_chars == 1.
	 * If max_chars == 0 we can drop through and pass it to simple_strntoull()
	 * and the content of *cp is irrelevant.
	 */
	if (*cp == '-' && max_chars > 0)
		return -simple_strntoull(cp + 1, max_chars - 1, endp, base);

	return simple_strntoull(cp, max_chars, endp, base);
}

/**
 * simple_strtoll - convert a string to a signed long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 *
 * This function has caveats. Please use kstrtoll instead.
 */
long long strtoll(const char *cp, char **endp, int base)
{
	return strntoll(cp, INT_MAX, endp, base);
}

/* 
 * strtod.c --
 *
 *	Source code for the "strtod" library procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * RCS: @(#) $Id$
 */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

static int maxExponent = 511;	/* Largest possible base 10 exponent.  Any
				 * exponent larger than this will already
				 * produce underflow or overflow, so there's
				 * no need to worry about additional digits.
				 */
static double powersOf10[] = {	/* Table giving binary powers of 10.  Entry */
    10.,			/* is 10^2^i.  Used to convert decimal */
    100.,			/* exponents into floating-point numbers. */
    1.0e4,
    1.0e8,
    1.0e16,
    1.0e32,
    1.0e64,
    1.0e128,
    1.0e256
};

/*
 *----------------------------------------------------------------------
 *
 * strtod --
 *
 *	This procedure converts a floating-point number from an ASCII
 *	decimal representation to internal double-precision format.
 *
 * Results:
 *	The return value is the double-precision floating-point
 *	representation of the characters in string.  If endPtr isn't
 *	NULL, then *endPtr is filled in with the address of the
 *	next character after the last one that was part of the
 *	floating-point number.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double strtod(const char * string, char **endPtr)
    /* const char *string;	A decimal ASCII floating-point number,
				 * optionally preceded by white space.
				 * Must have form "-I.FE-X", where I is the
				 * integer part of the mantissa, F is the
				 * fractional part of the mantissa, and X
				 * is the exponent.  Either of the signs
				 * may be "+", "-", or omitted.  Either I
				 * or F may be omitted, or both.  The decimal
				 * point isn't necessary unless F is present.
				 * The "E" may actually be an "e".  E and X
				 * may both be omitted (but not just one).
				 */
    /* char **endPtr;		If non-NULL, store terminating character's
				 * address here. */
{
    int sign, expSign = FALSE;
    double fraction, dblExp, *d;
    register const char *p;
    register int c;
    int exp = 0;		/* Exponent read from "EX" field. */
    int fracExp = 0;		/* Exponent that derives from the fractional
				 * part.  Under normal circumstatnces, it is
				 * the negative of the number of digits in F.
				 * However, if I is very long, the last digits
				 * of I get dropped (otherwise a long I with a
				 * large negative exponent could cause an
				 * unnecessary overflow on I alone).  In this
				 * case, fracExp is incremented one for each
				 * dropped digit. */
    int mantSize;		/* Number of digits in mantissa. */
    int decPt;			/* Number of mantissa digits BEFORE decimal
				 * point. */
    const char *pExp;		/* Temporarily holds location of exponent
				 * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = string;
    while (isspace(*p)) {
	p += 1;
    }
    if (*p == '-') {
	sign = TRUE;
	p += 1;
    } else {
	if (*p == '+') {
	    p += 1;
	}
	sign = FALSE;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0; ; mantSize += 1)
    {
	c = *p;
	if (!isdigit(c)) {
	    if ((c != '.') || (decPt >= 0)) {
		break;
	    }
	    decPt = mantSize;
	}
	p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */
    
    pExp  = p;
    p -= mantSize;
    if (decPt < 0) {
	decPt = mantSize;
    } else {
	mantSize -= 1;			/* One of the digits was the point. */
    }
    if (mantSize > 18) {
	fracExp = decPt - 18;
	mantSize = 18;
    } else {
	fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
	fraction = 0.0;
	p = string;
	goto done;
    } else {
	int frac1, frac2;
	frac1 = 0;
	for ( ; mantSize > 9; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac1 = 10*frac1 + (c - '0');
	}
	frac2 = 0;
	for (; mantSize > 0; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac2 = 10*frac2 + (c - '0');
	}
	fraction = (1.0e9 * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
	p += 1;
	if (*p == '-') {
	    expSign = TRUE;
	    p += 1;
	} else {
	    if (*p == '+') {
		p += 1;
	    }
	    expSign = FALSE;
	}
	while (isdigit(*p)) {
	    exp = exp * 10 + (*p - '0');
	    p += 1;
	}
    }
    if (expSign) {
	exp = fracExp - exp;
    } else {
	exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */
    
    if (exp < 0) {
	expSign = TRUE;
	exp = -exp;
    } else {
	expSign = FALSE;
    }
    if (exp > maxExponent) {
	exp = maxExponent;
    }
    dblExp = 1.0;
    for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
	if (exp & 01) {
	    dblExp *= *d;
	}
    }
    if (expSign) {
	fraction /= dblExp;
    } else {
	fraction *= dblExp;
    }

done:
    if (endPtr != NULL) {
	*endPtr = (char *) p;
    }

    if (sign) {
	return -fraction;
    }
    return fraction;
}
