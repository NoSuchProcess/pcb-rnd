/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/config.h>

#include <stdlib.h>
#include <string.h>

#define GVT_DONT_UNDEF
#include <librnd/core/color.h>
#include <genvector/genvector_impl.c>

#define PACK_COLOR_(r,g,b,a) ((((unsigned long)(r)) << 24) | (((unsigned long)(g)) << 16) | (((unsigned long)(b)) << 8) | (((unsigned long)(a))))
#define PACK_COLOR(clr) PACK_COLOR_(clr->r, clr->g, clr->b, clr->a)
#define COLOR_TO_FLOAT(clr) \
do { \
	clr->fr = (float)clr->r / 255.0; \
	clr->fg = (float)clr->g / 255.0; \
	clr->fb = (float)clr->b / 255.0; \
	clr->fa = (float)clr->a / 255.0; \
} while(0)

static const char *pcb_color_to_hex = "0123456789abcdef";
#define COLOR_TO_HEX(val) pcb_color_to_hex[(val) & 0x0F]

#define COLOR_TO_STR_COMP(dst, val) \
do { \
	(dst)[0] = COLOR_TO_HEX(val >> 4); \
	(dst)[1] = COLOR_TO_HEX(val); \
} while(0)

#define COLOR_TO_STR(clr) \
do { \
	clr->str[0] = '#'; \
	COLOR_TO_STR_COMP(clr->str+1, clr->r); \
	COLOR_TO_STR_COMP(clr->str+3, clr->g); \
	COLOR_TO_STR_COMP(clr->str+5, clr->b); \
	if (clr->a != 255) { \
		COLOR_TO_STR_COMP(clr->str+7, clr->a); \
		clr->str[9] = '\0'; \
	} \
	else \
		clr->str[7] = '\0'; \
} while(0)

#define CLAMP01(c) (((c) < 0.0) ? 0.0 : (((c) > 1.0) ? 1.0 : (c)))

int rnd_color_load_int(rnd_color_t *dst, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	dst->r = r;
	dst->g = g;
	dst->b = b;
	dst->a = a;
	dst->packed = PACK_COLOR(dst);
	COLOR_TO_FLOAT(dst);
	COLOR_TO_STR(dst);
	return 0;
}

int rnd_color_load_packed(rnd_color_t *dst, unsigned long p)
{
	dst->packed = p;
	dst->r = (p & 0xff000000UL) >> 24;
	dst->g = (p & 0x00ff0000UL) >> 16;
	dst->b = (p & 0x0000ff00UL) >> 8;
	dst->a = (p & 0x000000ffUL);
	COLOR_TO_FLOAT(dst);
	COLOR_TO_STR(dst);
	return 0;
}

int rnd_color_load_float(rnd_color_t *dst, float r, float g, float b, float a)
{
	dst->r = CLAMP01(r) * 255.0;
	dst->g = CLAMP01(g) * 255.0;
	dst->b = CLAMP01(b) * 255.0;
	dst->a = CLAMP01(a) * 255.0;
	dst->packed = PACK_COLOR(dst);
	COLOR_TO_STR(dst);
	return 0;
}

/* Convert a hex digit or return -1 on error */
#define HEXDIGCONV(res, chr) \
do { \
	if ((chr >= '0') && (chr <= '9')) res = chr - '0'; \
	else if ((chr >= 'a') && (chr <= 'f')) res = chr - 'a' + 10; \
	else if ((chr >= 'A') && (chr <= 'F')) res = chr - 'A' + 10; \
	else \
		return -1; \
} while(0)

/* convert a 2 digit hex number or return -1 on error */
#define HEXCONV(dst_fld, src, idx) \
do { \
	unsigned int tmp1, tmp2; \
	HEXDIGCONV(tmp1, src[idx]); \
	HEXDIGCONV(tmp2, src[idx+1]); \
	dst_fld = tmp1 << 4 | tmp2; \
} while(0)

int rnd_color_load_str(rnd_color_t *dst, const char *src)
{
	if (src[0] != '#')
		return -1;

	HEXCONV(dst->r, src, 1);
	HEXCONV(dst->g, src, 3);
	HEXCONV(dst->b, src, 5);
	if (src[7] != '\0')
		HEXCONV(dst->a, src, 7);
	else
		dst->a = 255;

	dst->packed = PACK_COLOR(dst);
	COLOR_TO_FLOAT(dst);
	COLOR_TO_STR(dst);
	return 0;
}

rnd_color_t *rnd_clrdup(const rnd_color_t *src)
{
	rnd_color_t *dst = malloc(sizeof(rnd_color_t));
	memcpy(dst, src, sizeof(rnd_color_t));
	return dst;
}

static rnd_color_t pcb_color_black_;
static rnd_color_t pcb_color_white_;
static rnd_color_t pcb_color_cyan_;
static rnd_color_t pcb_color_red_;
static rnd_color_t pcb_color_blue_;
static rnd_color_t pcb_color_drill_;
static rnd_color_t pcb_color_magenta_;
static rnd_color_t pcb_color_golden_;
static rnd_color_t pcb_color_grey33_;

const rnd_color_t *rnd_color_black = &pcb_color_black_;
const rnd_color_t *rnd_color_white = &pcb_color_white_;
const rnd_color_t *rnd_color_cyan = &pcb_color_cyan_;
const rnd_color_t *rnd_color_red = &pcb_color_red_;
const rnd_color_t *rnd_color_blue = &pcb_color_blue_;
const rnd_color_t *rnd_color_drill = &pcb_color_drill_;
const rnd_color_t *rnd_color_grey33 = &pcb_color_grey33_;
const rnd_color_t *rnd_color_magenta = &pcb_color_magenta_;
const rnd_color_t *rnd_color_golden = &pcb_color_golden_;

void rnd_color_init(void)
{
	rnd_color_load_str(&pcb_color_black_, "#000000");
	rnd_color_load_str(&pcb_color_white_, "#ffffff");
	rnd_color_load_str(&pcb_color_cyan_, "#00ffff");
	rnd_color_load_str(&pcb_color_red_, "#ff0000");
	rnd_color_load_str(&pcb_color_blue_, "#0000ff");
	rnd_color_load_str(&pcb_color_grey33_, "#333333");
	rnd_color_load_str(&pcb_color_magenta_, "#ff00ff");
	rnd_color_load_str(&pcb_color_golden_, "#dddd22");
	rnd_color_load_str(&pcb_color_drill_, "#ff00ff");
	strcpy(pcb_color_drill_.str, "drill");
}


