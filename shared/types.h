/*
 * types.h
 *
 * Copyright (C) 2002 Simon Nieuviarts
 *
 * System types.
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
#ifndef __IA32_TYPES_H__
#define __IA32_TYPES_H__

#ifndef __SIZE_TYPE__
#error __SIZE_TYPE__ not defined
#endif

typedef __SIZE_TYPE__ size_t;

#ifndef __PTRDIFF_TYPE__
#error __PTRDIFF_TYPE__ not defined
#endif

typedef __PTRDIFF_TYPE__ ptrdiff_t;

#endif
