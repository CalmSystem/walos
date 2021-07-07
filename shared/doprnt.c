/*
 * Copyright (c) 1994,1996-1999 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#define FALSE ((unsigned char) 0)
#define TRUE ((unsigned char) 1)
#include "stdarg.h"
#include "string.h"	/* strlen */
#include "doprnt.h"

/*
 *  Common code for printf et al.
 *
 *  The calling routine typically takes a variable number of arguments,
 *  and passes the address of the first one.  This implementation
 *  assumes a straightforward, stack implementation, aligned to the
 *  machine's wordsize.  Increasing addresses are assumed to point to
 *  successive arguments (left-to-right), as is the case for a machine
 *  with a downward-growing stack with arguments pushed right-to-left.
 *
 *  To write, for example, fprintf() using this routine, the code
 *
 *	fprintf(fd, format, args)
 *	FILE *fd;
 *	char *format;
 *	{
 *	_doprnt(format, &args, fd);
 *	}
 *
 *  would suffice.  (This example does not handle the fprintf's "return
 *  value" correctly, but who looks at the return value of fprintf
 *  anyway?)
 *
 *  This version implements the following printf features:
 *
 *	%d	decimal conversion
 *	%u	unsigned conversion
 *	%x	hexadecimal conversion
 *	%X	hexadecimal conversion with capital letters
 *	%o	octal conversion
 *	%c	character
 *	%s	string
 *	%m.n	field width, precision
 *	%-m.n	left adjustment
 *	%0m.n	zero-padding
 *	%*.*	width and precision taken from arguments
 *
 *  This version does not implement %f, %e, or %g.  It accepts, but
 *  ignores, an `l' as in %ld, %lo, %lx, and %lu, and therefore will not
 *  work correctly on machines for which sizeof(long) != sizeof(int).
 *  It does not even parse %D, %O, or %U; you should be using %ld, %o and
 *  %lu if you mean long conversion.
 *
 *  As mentioned, this version does not return any reasonable value.
 *
 *  Permission is granted to use, modify, or propagate this code as
 *  long as this notice is incorporated.
 *
 *  Steve Summit 3/25/87
 */

/*
 * Added formats for decoding device registers:
 *
 * printf("reg = %b", regval, "<base><arg>*")
 *
 * where <base> is the output base expressed as a control character:
 * i.e. '\10' gives octal, '\20' gives hex.  Each <arg> is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the rest (up to a control character (<= 32)) give the
 * name of the register.  Thus
 *	printf("reg = %b\n", 3, "\10\2BITTWO\1BITONE")
 * would produce
 *	reg = 3<BITTWO,BITONE>
 *
 * If the second character in <arg> is also a control character, it
 * indicates the last bit of a bit field.  In this case, printf will extract
 * bits <1> to <2> and print it.  Characters following the second control
 * character are printed before the bit field.
 *	printf("reg = %b\n", 0xb, "\10\4\3FIELD1=\2BITTWO\1BITONE")
 * would produce
 *	reg = b<FIELD1=2,BITONE>
 */
/*
 * Added for general use:
 *	#	prefix for alternate format:
 *		0x (0X) for hex
 *		leading 0 for octal
 *	+	print '+' if positive
 *	blank	print ' ' if positive
 *
 *	z	signed hexadecimal
 *	r	signed, 'radix'
 *	n	unsigned, 'radix'
 *
 *	D,U,O,Z	same as corresponding lower-case versions
 *		(compatibility)
 */
/*
 *	Added ANSI %p for pointers.  Output looks like 0x%08x.
 */

#define isdigit(d) ((d) >= '0' && (d) <= '9')
#define Ctod(c) ((c) - '0')

#define MAXBUF (sizeof(long int) * 8)		 /* enough for binary */

static void
printnum(u, base, putc, putc_arg)
	register unsigned long	u;	/* number to print */
	register int		base;
	void			(*putc)();
	char			*putc_arg;
{
	char	buf[MAXBUF];	/* build number here */
	register char *	p = &buf[MAXBUF-1];
	static char digs[] = "0123456789abcdef";

	do {
	    *p-- = digs[u % base];
	    u /= base;
	} while (u != 0);

	while (++p != &buf[MAXBUF])
	    (*putc)(putc_arg, *p);
}

static unsigned char	_doprnt_truncates = FALSE;

void _doprnt(fmt, args, radix, putc, putc_arg)
	const char 	*fmt;
	va_list		args;
	int		radix;		/* default radix - for '%r' */
 	void		(*putc)();	/* character output */
	char		*putc_arg;	/* argument for putc */
{
	int		length;
	int		prec;
	unsigned char	ladjust;
	char		padc;
	long		n;
	unsigned long	u;
	int		plus_sign;
	int		sign_char;
	unsigned char	altfmt, truncate;
	int		base;
	char		c;
#ifdef DOPRNT_FLOATS
	int		float_hack;
	char		*p;

	/* Copy the fmt so we can modify it in immoral ways. */
	p = alloca(strlen(fmt) + 1);
	strcpy(p, fmt);
	fmt = p;
#endif

	while (*fmt != '\0') {
	    if (*fmt != '%') {
		(*putc)(putc_arg, *fmt++);
		continue;
	    }

	    fmt++;

	    length = 0;
	    prec = -1;
	    ladjust = FALSE;
	    padc = ' ';
	    plus_sign = 0;
	    sign_char = 0;
	    altfmt = FALSE;

	    while (TRUE) {
		if (*fmt == '#') {
		    altfmt = TRUE;
		    fmt++;
		}
		else if (*fmt == '-') {
		    ladjust = TRUE;
		    fmt++;
		}
		else if (*fmt == '+') {
		    plus_sign = '+';
		    fmt++;
		}
		else if (*fmt == ' ') {
		    if (plus_sign == 0)
			plus_sign = ' ';
		    fmt++;
		}
		else
		    break;
	    }

	    if (*fmt == '0') {
		padc = '0';
		fmt++;
	    }

	    if (isdigit(*fmt)) {
		while(isdigit(*fmt))
		    length = 10 * length + Ctod(*fmt++);
	    }
	    else if (*fmt == '*') {
		length = va_arg(args, int);
		fmt++;
		if (length < 0) {
		    ladjust = !ladjust;
		    length = -length;
		}
	    }

	    if (*fmt == '.') {
		fmt++;
		if (isdigit(*fmt)) {
		    prec = 0;
		    while(isdigit(*fmt))
			prec = 10 * prec + Ctod(*fmt++);
		}
		else if (*fmt == '*') {
		    prec = va_arg(args, int);
		    fmt++;
		}
	    }

	    if (*fmt == 'l')
		fmt++;	/* need it if sizeof(int) < sizeof(long) */

	    truncate = FALSE;
#ifdef DOPRNT_FLOATS
	    float_hack = FALSE;
#endif

	    switch(*fmt) {
		case 'b':
		case 'B':
		{
		    register char *p;
		    unsigned char	  any;
		    register int  i;

		    u = va_arg(args, unsigned long);
		    p = va_arg(args, char *);
		    base = *p++;
		    printnum(u, base, putc, putc_arg);

		    if (u == 0)
			break;

		    any = FALSE;
		    while ((i = *p++) != 0) {
			/* NOTE: The '32' here is because ascii space */
			if (*p <= 32) {
			    /*
			     * Bit field
			     */
			    register int j;
			    if (any)
				(*putc)(putc_arg, ',');
			    else {
				(*putc)(putc_arg, '<');
				any = TRUE;
			    }
			    j = *p++;
			    for (; (c = *p) > 32; p++)
				(*putc)(putc_arg, c);
			    printnum((unsigned)( (u>>(j-1)) & ((2<<(i-j))-1)),
					base, putc, putc_arg);
			}
			else if (u & (1<<(i-1))) {
			    if (any)
				(*putc)(putc_arg, ',');
			    else {
				(*putc)(putc_arg, '<');
				any = TRUE;
			    }
			    for (; (c = *p) > 32; p++)
				(*putc)(putc_arg, c);
			}
			else {
			    for (; *p > 32; p++)
				continue;
			}
		    }
		    if (any)
			(*putc)(putc_arg, '>');
		    break;
		}

		case 'c':
		    c = va_arg(args, int);
		    (*putc)(putc_arg, c);
		    break;

		case 's':
		{
		    register char *p;
		    register char *p2;

		    if (prec == -1)
			prec = 0x7fffffff;	/* MAXINT */

		    p = va_arg(args, char *);

		    if (p == (char *)0)
			p = "";

		    if (length > 0 && !ladjust) {
			n = 0;
			p2 = p;

			for (; *p != '\0' && n < prec; p++)
			    n++;

			p = p2;

			while (n < length) {
			    (*putc)(putc_arg, ' ');
			    n++;
			}
		    }

		    n = 0;

		    while (*p != '\0') {
			if (++n > prec)
			    break;

			(*putc)(putc_arg, *p++);
		    }

		    if (n < length && ladjust) {
			while (n < length) {
			    (*putc)(putc_arg, ' ');
			    n++;
			}
		    }

		    break;
		}

		case 'o':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'O':
		    base = 8;
		    goto print_unsigned;

		case 'd':
	        case 'i':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'D':
		    base = 10;
		    goto print_signed;

#ifdef DOPRNT_FLOATS
		/*
		 * This is horrible and needs to be redone sometime.
		 */
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'e':
		case 'E':
 		    truncate = _doprnt_truncates;
		    base = 10;
		    {
			static int first = 1;
			static int oldprec = 6;
			static char oldchar;
			double x;
			int    i;

			/* we don't do left adjustment, sorry */
			ladjust = 0;

			if (first) {
			    double dd = 1;

			    x = va_arg(args, double);
			    n = (int)x;
			    sign_char = n >= 0 ? plus_sign : '-';
			    u = n >= 0 ? n : -n;

			    /* default prec is six digits */
			    if (prec == -1)
				prec = 6;
			    oldprec = prec;

			    length -= (prec + 1);

			    /* rewind args and fmt */
			    args -= __va_size(double);

			    /* uh-oh! we write the format string... */
			    /* But now it's in an array we allocated.  */
			    oldchar = fmt[-1];
			    fmt[-1] = '%';
			    fmt -= 2;

			    /* stick fraction on arg */
			    /* compute multiplier */
			    for (i = 0; i < prec; i++)
				dd *= 10.0;

			    x -= (double)((int)x);
			    x *= dd;
			    *((double *)args) = x;

			} else {
			    /* restore old stuff */
			    fmt[-1] = oldchar;

			    x = va_arg(args, double);
			    n = (int)x;
			    sign_char = '.';
			    u = n >= 0 ? n : -n;;

			    length = oldprec + 1; /* Need to account for the . */
			    float_hack = 1;
			    ladjust = 0;
			    padc = '0';
			}
			first = !first;
		    }
		    goto print_num;
#endif /* DOPRNT_FLOATS */

		case 'u':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'U':
		    base = 10;
		    goto print_unsigned;

		case 'p':
		    padc = '0';
		    length = 8;
		    /*
		     * We do this instead of just setting altfmt to TRUE
		     * because we want 0 to have a 0x in front, and we want
		     * eight digits after the 0x -- not just 6.
		     */
		    (*putc)(putc_arg, '0');
		    (*putc)(putc_arg, 'x');
		    /* FALLTHROUGH */
		case 'x':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'X':
		    base = 16;
		    goto print_unsigned;

		case 'z':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'Z':
		    base = 16;
		    goto print_signed;

		case 'r':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'R':
		    base = radix;
		    goto print_signed;

		case 'n':
 		    truncate = _doprnt_truncates;
		    /* FALLTHROUGH */
		case 'N':
		    base = radix;
		    goto print_unsigned;

		print_signed:
		    n = va_arg(args, long);
		    if (n >= 0) {
			u = n;
			sign_char = plus_sign;
		    }
		    else {
			u = -n;
			sign_char = '-';
		    }
		    goto print_num;

		print_unsigned:
		    u = va_arg(args, unsigned long);
		    goto print_num;

		print_num:
		{
		    char	buf[MAXBUF];	/* build number here */
		    register char *	p = &buf[MAXBUF-1];
		    static char digits[] = "0123456789abcdef";
		    char *prefix = 0;

		    if (truncate) u = (long)((int)(u));

		    if (u != 0 && altfmt) {
			if (base == 8)
			    prefix = "0";
			else if (base == 16)
			    prefix = "0x";
		    }

		    do {
			*p-- = digits[u % base];
			u /= base;
			prec--;
		    } while (u != 0);

		    length -= (&buf[MAXBUF-1] - p);
		    if (sign_char)
			length--;
		    if (prefix)
			length -= strlen(prefix);

		    if (prec > 0)
			    length -= prec;
		    if (padc == ' ' && !ladjust) {
			/* blank padding goes before prefix */
			while (--length >= 0)
			    (*putc)(putc_arg, ' ');
		    }
		    if (sign_char)
			(*putc)(putc_arg, sign_char);
		    if (prefix)
			while (*prefix)
			    (*putc)(putc_arg, *prefix++);

		    while (--prec >= 0)
			    (*putc)(putc_arg, '0');

		    if (padc == '0') {
			/* zero padding goes after sign and prefix */
			while (--length >= 0)
			    (*putc)(putc_arg, '0');
		    }
		    while (++p != &buf[MAXBUF])
			(*putc)(putc_arg, *p);

#ifdef DOPRNT_FLOATS
		    if (ladjust) {
			while (--length >= 0)
			    (*putc)(putc_arg, float_hack? '0' : ' ');
		    }
		    float_hack = 0;
#else
		    if (ladjust) {
			while (--length >= 0)
			    (*putc)(putc_arg, ' ');
		    }
#endif
		    break;
		}

		case '\0':
		    fmt--;
		    break;

		default:
		    (*putc)(putc_arg, *fmt);
	    }
	fmt++;
	}
}
