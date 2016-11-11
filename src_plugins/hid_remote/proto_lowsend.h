/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "base64.h"

#define SUBSEQ_MASK (1 << (ctx->depth - 1))

static int send_raw(proto_ctx_t *ctx, const void *data, size_t len)
{
	return write(1, data, len);
}

#define PREFIX_LEN 10
static int sendf(proto_ctx_t *ctx, const char *fmt, ...)
{
	va_list ap;
	char buff[8192];
	char *out_start, *payload = buff + PREFIX_LEN;
	size_t slen, len, payload_len = sizeof(buff) - PREFIX_LEN;

	va_start(ap, fmt);
	len = proto_vsnprintf(payload, payload_len, fmt, ap);
	if (len >= payload_len-1)
		return -1; /* too long, won't split for now */
	va_end(ap);

	if (ctx->bindepth > 0) {
		size_t blen;
		payload[-1] = '='; /* separator */
		blen = base64_write_right(buff, PREFIX_LEN-2, len);
		if (blen <= 0)
			return -1;
		blen++; /* for the separator */
		out_start = payload - blen;
		len += blen;
	}
	else
		out_start = payload;

	if (ctx->subseq_tab & SUBSEQ_MASK) {
		/* subsequent field on the given level, insert a space */
		out_start--;
		*out_start = ' ';
		len++;
	}

	slen = send_raw(ctx, out_start, len);

	/* next on this field will be subsequent */
	ctx->subseq_tab |= SUBSEQ_MASK;

	if (slen != len)
		return -1;
	return 0;
}


static int send_open(proto_ctx_t *ctx, int bin)
{
	int res;

	if (bin || (ctx->bindepth > 0)) {
		res = send_raw(ctx, "{", 1);
		ctx->bindepth++;
	}
	else
		res = send_raw(ctx, "(", 1);
	ctx->depth++;
	ctx->subseq_tab &= ~SUBSEQ_MASK;

	if (res != 1)
		return -1;
	return 0;
}

static int send_close(proto_ctx_t *ctx)
{
	int res;
	if (ctx->bindepth > 0) {
		res = send_raw(ctx, "}", 1);
		ctx->bindepth--;
	}
	else
		res = send_raw(ctx, ")", 1);
	ctx->depth--;

	if (res != 1)
		return -1;
	return 0;
}

static int send_begin(proto_ctx_t *ctx, const char *s)
{
	ctx->depth = 0;
	ctx->bindepth = 0;
	return send_raw(ctx, s, strlen(s));
}

static int send_end(proto_ctx_t *ctx)
{
	while(ctx->depth > 0)
		if (send_close(ctx) < 0)
			return -1;
	return send_raw(ctx, "\n", 1);
}

/* Returns whether str needs to be sent as binary */
static int str_is_bin(const char *str)
{
	const char *s;
	int l;
	if ((str == NULL) || (*str == '\0')) /* empty string can be encoded in bin only */
		return 1;
	for(s = str, l = 0; *s != '\0'; s++,l++)
		if ((l > 16) && (chr_is_bin(*s)))
			return 1;
	return 0;
}
