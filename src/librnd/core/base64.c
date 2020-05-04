/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015,2020 Tibor 'Igor2' Palinkas
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
#include <limits.h>
#include <librnd/core/base64.h>

#define PAD '='

static char int2digit(int digit)
{
	if (digit < 26) return 'A' + digit;
	if (digit < 52) return 'a' + digit - 26;
	if (digit < 62) return '0' + digit - 52;
	if (digit == 62) return '+';
	if (digit == 63) return '/';
	return '\0'; /* error */
}

static int digit2int(char c)
{
	if ((c >= 'A') && (c <= 'Z')) return c - 'A';
	if ((c >= 'a') && (c <= 'z')) return c - 'a' + 26;
	if ((c >= '0') && (c <= '9')) return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}


size_t rnd_base64_write_right(char *buff_start, size_t buff_len, unsigned long int num)
{
	char *end = buff_start + buff_len;
	size_t rlen = 0;

	if (num == 0) {
		*end = int2digit(0);
		return 1;
	}

	while(num > 0) {
		unsigned int digit = num & 0x3F;
		*end = int2digit(digit);
		num >>= 6;
		rlen++;
	}

	return rlen;
}

int rnd_base64_parse_grow(unsigned long int *num, int chr, int term)
{
	int digit;
	if (chr == term)
		return 1;

	if (*num >= (ULONG_MAX >> 6UL))
		return -1;

	digit = digit2int(chr);
	if (digit < 0)
		return -1;

	(*num) <<= 6UL;
	(*num) |= digit;
	return 0;
}

size_t rnd_base64_bin2str(char *dst, size_t dst_len, const unsigned char *src, size_t src_len)
{
	unsigned int val = 0, tmp;
	int bits = 0;
	size_t olen = 0;

	if (dst_len == 0)
		return -1;

	while((bits > 0) || (src_len > 0)) {
		if ((bits < 6) && (src_len > 0)) {
			tmp = *src;
			src++;
			src_len--;
			bits += 8;
			val <<= 8;
			val |= tmp;
		}
		if (bits < 6) {
			val = val << (6-bits);
			bits = 6;
		}
		tmp = (val >> (bits-6)) & 0x3F;
		*dst++ = int2digit(tmp);
		bits -= 6;
		olen++;
		if (olen >= dst_len)
			return -1;
	}

	while((olen % 4) != 0) {
		*dst++ = PAD;
		olen++;
		if (olen >= dst_len)
			return -1;
	}

	return olen;
}

size_t rnd_base64_str2bin(unsigned char *dst, size_t dst_len, const char *src, size_t src_len)
{
	unsigned int val, tmp;
	int bits = 0;
	size_t olen = 0;

	while(src_len > 0) {
		while(bits < 8) {
			if (*src != '=') {
				if (src_len <= 0)
					return -1;
				tmp = digit2int(*src);
				src++;
				src_len--;
			}
			else {
				tmp = 0;
				return olen;
			}
			val <<= 6;
			val |= tmp;
			bits += 6;
		}
		tmp = (val >> (bits-8)) & 0xFF;
		*dst = tmp;
		dst++;
		olen++;
		bits -= 8;
		if (olen >= dst_len)
			return -1;
	}

	return olen;
}
