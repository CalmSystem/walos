/*
 * debug.h
 *
 * Copyright (C) 2003 Simon Nieuviarts
 *
 * Functions to help writing on the screen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "stddef.h"
#include "stdarg.h"

int printf(const char *__format, ...) __attribute__((format (printf, 1, 2)));
int vprintf(const char *__format, va_list __vl) __attribute__((format (printf, 1, 0)));
int sprintf(char *__dest, const char *__format, ...) __attribute__((format (printf, 2, 3)));
int snprintf(char *__dest, size_t __size, const char *__format, ...) __attribute__((format (printf, 3, 4)));
int vsprintf(char *__dest, const char *__format, va_list __vl) __attribute__((format (printf, 2, 0)));
int vsnprintf(char *__dest, size_t __size, const char *__format, va_list __vl) __attribute__((format (printf, 3, 0)));

extern void panic(const char *fmt, ...) __attribute__((noreturn, format (printf, 1, 2)));

#define BUG() do { panic(__FILE__":%u: BUG !\n", __LINE__); } while (0)

#ifdef NDEBUG

#define assert(expr) ((void)0)

#else

#define assert(expr) \
	((void)((expr) ? 0 : \
		(panic(__FILE__":%u: failed assertion `"#expr"'\n", \
			__LINE__), 0)))

#endif

#endif
