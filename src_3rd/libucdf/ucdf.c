/*

ucdf - microlib for reading Compound Document Format files

Copyright (c) 2021 Tibor 'Igor2' Palinkas
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Author nor the names of contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Source code: svn://svn.repo.hu/pcb-rnd/trunk
Contact the author: http://igor2.repo.hu/contact.html

*/

#include <stdlib.h>
#include <string.h>
#include "ucdf.h"

const char *ucdf_error_str[] = {
	"success",
	"failed to open file",
	"failed to read",
	"wrong file ID",
	"broken file header",
	"broken MSAT chain",
	"broken SAT chain",
	"broken short SAT chain",
	NULL
};

static const unsigned char hdr_id[8] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };

#define safe_read(dst, len) \
	do { \
		if (fread(dst, 1, len, ctx->f) != len) { \
			ctx->error = UCDF_ERR_READ; \
			return -1; \
		} \
	} while(0)

#define safe_seek(offs) \
	do { \
		if (fseek(ctx->f, offs, SEEK_SET) != 0) { \
			ctx->error = UCDF_ERR_READ; \
			return -1; \
		} \
	} while(0)

#define error(code) \
	do { \
		ctx->error = code; \
		return -1; \
	} while(0)

static long sect_id2offs(ucdf_file_t *ctx, long id)
{
	return 512L + (id << ctx->ssz);
}

static long load_int(ucdf_file_t *ctx, unsigned const char *src, int len)
{
	int n, sh;
	long tmp, res = 0;

	if (ctx->litend) {
		for(sh = n = 0; n < len; n++,sh += 8,src++) {
			tmp = *src;
			tmp = (tmp << sh);
			res |= tmp;
		}
	}
	else {
		for(; len > 0; len--,src++) {
			res <<= 8;
			res |= *src;
		}
	}

	/* "sign extend" */
	if (res >= (1UL << (len*8-1)))
		res = -((1UL << (len*8)) - res);

	return res;
}

static int ucdf_read_hdr(ucdf_file_t *ctx)
{
	unsigned char buff[16];

	safe_read(buff, sizeof(hdr_id));
	if (memcmp(buff, hdr_id, sizeof(hdr_id)) != 0)
		error(UCDF_ERR_BAD_ID);

	/* figure the byte order @28 */
	safe_seek(28);
	safe_read(buff, 2);
	if ((buff[0] == 0xFE) || (buff[1] == 0xFF))
		ctx->litend = 1;
	else if ((buff[0] == 0xFF) || (buff[1] == 0xFE))
		ctx->litend = 0;
	else
		error(UCDF_ERR_BAD_HDR);

	/* sector sizes @30 */
	safe_read(buff, 2);
	ctx->ssz = load_int(ctx, buff, 2);
	if (ctx->ssz < 7)
		error(UCDF_ERR_BAD_HDR);
	ctx->sect_size = 1 << ctx->ssz;
	safe_read(buff, 2);
	ctx->sssz = load_int(ctx, buff, 2);
	if (ctx->sssz > ctx->ssz)
		error(UCDF_ERR_BAD_HDR);
	ctx->short_sect_size = 1 << ctx->sssz;

	/* allocation table info @44 */
	safe_seek(44);
	safe_read(buff, 4);
	ctx->sat_len = load_int(ctx, buff, 4);
	safe_read(buff, 4);
	ctx->dir_first = load_int(ctx, buff, 4);

	safe_seek(60);
	safe_read(buff, 4);
	ctx->ssat_first = load_int(ctx, buff, 4);
	safe_read(buff, 4);
	ctx->ssat_len = load_int(ctx, buff, 4);
	safe_read(buff, 4);
	ctx->msat_first = load_int(ctx, buff, 4);
	safe_read(buff, 4);
	ctx->msat_len = load_int(ctx, buff, 4);

	/* versions @24 */
	safe_seek(24);
	safe_read(buff, 2);
	ctx->file_rev = load_int(ctx, buff, 2);
	safe_read(buff, 2);
	ctx->file_ver = load_int(ctx, buff, 2);


	return 0;
}

static int ucdf_load_any_sat(ucdf_file_t *ctx, long *dst, long *idx)
{
	int n;
	long id_per_sect = ctx->sect_size >> 2;
	unsigned char buff[4];

	for(n = 0; n < id_per_sect; n++) {
		safe_read(buff, 4);
		dst[*idx] = load_int(ctx, buff, 4);
/*		printf(" [%ld]: sect %ld (%02x %02x %02x %02x)\n", *idx, ctx->msat[*idx], buff[0], buff[1], buff[2], buff[3]);*/
		(*idx)++;
	}
	return 0;
}

static int ucdf_load_msat_(ucdf_file_t *ctx, long num_ids, long *idx)
{
	int n;
	unsigned char buff[4];

	for(n = 0; n < num_ids; n++) {
		safe_read(buff, 4);
		ctx->msat[*idx] = load_int(ctx, buff, 4);
/*		printf(" [%ld]: sect %ld (%02x %02x %02x %02x)\n", *idx, ctx->msat[*idx], buff[0], buff[1], buff[2], buff[3]);*/
		(*idx)++;
	}
	return 0;
}

static long ucdf_load_msat(ucdf_file_t *ctx, long num_ids, long *idx)
{
	unsigned char buff[4];
	long next;

	/* load content */
	if (ucdf_load_msat_(ctx, num_ids-1, idx) != 0)
		return -1;

	/* load next sector address in the chain */
	safe_read(buff, 4);
	next = load_int(ctx, buff, 4);

	printf("next sect: %ld\n", next);
	return next;
}

static int ucdf_read_msat(ucdf_file_t *ctx)
{
	long next, n, idx = 0, id_per_sect = ctx->sect_size >> 2;

	ctx->msat = malloc(sizeof(long) * (ctx->msat_len * id_per_sect + 109));

	/* load first block of msat from the header @ 76*/
	safe_seek(76);
	ucdf_load_msat_(ctx, 109, &idx);

	/* load further msat blocks */
	next = ctx->msat_first;
	for(n = 0; n < ctx->msat_len; n++) {
		safe_seek(sect_id2offs(ctx, next));
		next = ucdf_load_msat(ctx, id_per_sect, &idx);
		if (next < 0)
			break;
	}

	if ((n != ctx->msat_len) || (next != UCDF_SECT_EOC))
		error(UCDF_ERR_BAD_MSAT);

	/* load and build the sat */
	ctx->sat = malloc(sizeof(long) * ctx->sat_len * id_per_sect);
	idx = 0;
	for(n = 0; n < ctx->sat_len; n++) {
		next = ctx->msat[n];
		if (next < 0)
			error(UCDF_ERR_BAD_SAT);
		safe_seek(sect_id2offs(ctx, next));
		if (ucdf_load_any_sat(ctx, ctx->sat, &idx) != 0)
			return -1;
	}

	/* load and build the short sat */
	ctx->ssat = malloc(sizeof(long) * ctx->ssat_len * id_per_sect);
	idx = 0;
	for(n = 0; n < ctx->ssat_len; n++) {
		next = ctx->msat[n];
		if (next < 0)
			error(UCDF_ERR_BAD_SSAT);
		safe_seek(sect_id2offs(ctx, next));
		if (ucdf_load_any_sat(ctx, ctx->ssat, &idx) != 0)
			return -1;
	}

	return 0;
}

int ucdf_open(ucdf_file_t *ctx, const char *path)
{
	ctx->f = fopen(path, "rb");
	if (ctx->f == NULL) {
		ctx->error = UCDF_ERR_OPEN;
		return -1;
	}

	if (ucdf_read_hdr(ctx) != 0)
		goto error;

	if (ucdf_read_msat(ctx) != 0)
		goto error;

	return 0;

	error:;
	fclose(ctx->f);
	ctx->f = NULL;
	return -1;
}
