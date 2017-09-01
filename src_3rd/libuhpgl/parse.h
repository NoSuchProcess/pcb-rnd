/*
    libuhpgl - the micro HP-GL library
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


    This library is part of pcb-rnd: http://repo.hu/projects/pcb-rnd
*/


#ifndef LIBUHPGL_PRASE_H
#define LIBUHPGL_PRASE_H

#include <stdio.h>
#include "libuhpgl.h"

/* Shall be called before parsing starts (call uhpgl_parse_open() at the end) */
int uhpgl_parse_open(uhpgl_ctx_t *ctx);

/* Parse next character of a stream; c may be EOF; returns 0 on success */
int uhpgl_parse_char(uhpgl_ctx_t *ctx, int c);

/* Parse next portion of a stream from a string; returns 0 on success */
int uhpgl_parse_str(uhpgl_ctx_t *ctx, const char *str);

/* Parse next portion of a stream from a file; returns 0 on success */
int uhpgl_parse_file(uhpgl_ctx_t *ctx, FILE *f);

/* Shall be called after the last portion parsed; returns 0 on success */
int uhpgl_parse_close(uhpgl_ctx_t *ctx);

#endif
