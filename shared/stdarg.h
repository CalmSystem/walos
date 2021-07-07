/*
 * stdarg.h
 *
 * Copyright (C) 2001 Simon Nieuviarts
 *
 * We like variable argument lists.
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

#ifndef __STDARG_H__
#define __STDARG_H__

typedef __builtin_va_list va_list;
#define va_start(v,l)   __builtin_va_start((v),l)
#define va_end          __builtin_va_end
#define va_arg          __builtin_va_arg

#endif
