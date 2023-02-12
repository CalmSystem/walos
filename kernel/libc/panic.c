/*
 * panic.c
 *
 * Copyright (C) 2003 Simon Nieuviarts
 *
 * When kernel does not know what to do more clever.
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

#include "stdio.h"
#include "stdarg.h"

void __attribute__((noreturn)) panic(const char *fmt, ...)
{
	va_list ap;

	printf("PANIC: ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	__builtin_trap();
}

void __attribute__((noreturn)) abort(void)
{
	__builtin_trap();
}
