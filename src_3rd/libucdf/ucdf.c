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
	"broken direcotry chain",
	"failed to allocate memory",
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

static long sect_id2offs(ucdf_ctx_t *ctx, long id)
{
	return 512L + (id << ctx->ssz);
}

static long load_int(ucdf_ctx_t *ctx, unsigned const char *src, int len)
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

static int ucdf_read_hdr(ucdf_ctx_t *ctx)
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

	safe_seek(56);
	safe_read(buff, 4);
	ctx->long_stream_min_size = load_int(ctx, buff, 4);
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

static int ucdf_load_any_sat(ucdf_ctx_t *ctx, long *dst, long *idx)
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

static int ucdf_load_msat_(ucdf_ctx_t *ctx, long num_ids, long *idx)
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

static long ucdf_load_msat(ucdf_ctx_t *ctx, long num_ids, long *idx)
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

static int ucdf_read_sats(ucdf_ctx_t *ctx)
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
	ctx->ssat = calloc(sizeof(long) * ctx->ssat_len * id_per_sect, 1);
	next = ctx->ssat_first;
	idx = 0;
	for(n = 0; n < ctx->ssat_len; n++) {
/*		printf("ssat next=%ld seek=%ld\n", next, sect_id2offs(ctx, next));*/
		safe_seek(sect_id2offs(ctx, next));
		if (ucdf_load_any_sat(ctx, ctx->ssat, &idx) != 0)
			return -1;
		next = ctx->sat[next];
	}
	if (next != UCDF_SECT_EOC)
		error(UCDF_ERR_BAD_SSAT);

	return 0;
}

static void utf16_to_ascii(char *dst, int dstlen, const unsigned char *src)
{
	for(dstlen--; (*src != '\0') && (dstlen > 0); dstlen--, dst++, src+=2)
		*dst = *src;
	*dst = '\0';
}

typedef struct {
	long num_dirsects, dirs_per_sect;
	long *diroffs;
} dir_ctx_t;

static int ucdf_read_dir(ucdf_ctx_t *ctx, dir_ctx_t *dctx, long dirid, ucdf_direntry_t *parent, ucdf_direntry_t **de_out)
{
	long page = dirid / dctx->dirs_per_sect;
	long offs = dirid % dctx->dirs_per_sect;
	long leftid, rightid, rootid;
	unsigned char buf[128];
	ucdf_direntry_t *de = malloc(sizeof(ucdf_direntry_t));

	if ((page < 0) || (page >= dctx->num_dirsects))
		error(UCDF_ERR_BAD_DIRCHAIN);

	safe_seek(dctx->diroffs[page] + offs * 128);
	safe_read(buf, 128);

	utf16_to_ascii(de->name, sizeof(de->name), buf);
	de->size  = load_int(ctx, buf+120, 4);
	de->is_short = de->size < ctx->long_stream_min_size;
	leftid  = load_int(ctx, buf+68, 4);
	rightid = load_int(ctx, buf+72, 4);
	rootid = load_int(ctx, buf+76, 4);
	de->first = load_int(ctx, buf+116, 4);
	de->type = buf[66];
	de->parent = parent;
	de->children = NULL;

	if (parent != NULL) {
		de->next = parent->children;
		parent->children = de;
	}
	else
		de->next = NULL;

	if (de_out != NULL)
		*de_out = de;

/*	printf("dir: %d:'%s' (%ld %ld r=%ld) sec=%ld\n", de->type, de->name, leftid, rightid, rootid, de->first);*/

	if (leftid >= 0)
		ucdf_read_dir(ctx, dctx, leftid, de->parent, NULL);
	if (rightid >= 0)
		ucdf_read_dir(ctx, dctx, rightid, de->parent, NULL);
	if (rootid >= 0)
		ucdf_read_dir(ctx, dctx, rootid, de, NULL);

	return 0;
}

static int ucdf_read_dirs(ucdf_ctx_t *ctx)
{
	int res;
	long n, next;
	dir_ctx_t dctx;

	/* count directory sectors */
	next = ctx->dir_first;
	dctx.num_dirsects = 0;
	dctx.dirs_per_sect = ctx->sect_size >> 7;
	while(next >= 0) {
		next = ctx->sat[next];
		dctx.num_dirsects++;
	}
	if (next != UCDF_SECT_EOC)
		error(UCDF_ERR_BAD_DIRCHAIN);

	/* build a temporary offset list of dir pages */
	dctx.diroffs = malloc(sizeof(long) * dctx.num_dirsects);
	if (dctx.diroffs == NULL)
		error(UCDF_ERR_BAD_MALLOC);

	next = ctx->dir_first;
	n = 0;
	while(next >= 0) {
		dctx.diroffs[n] = sect_id2offs(ctx, next);
		next = ctx->sat[next];
		n++;
	}

	/* start reading from the root dir */
	res = ucdf_read_dir(ctx, &dctx, 0, NULL, &ctx->root);

	free(dctx.diroffs);
	return res;
}

/* Set up the virtual long file that holds short sector data */
static int ucdf_setup_ssd(ucdf_ctx_t *ctx)
{
	long id_per_sect = ctx->sect_size >> 2;

	if (ctx->root->type != UCDF_DE_ROOT)
		error(UCDF_ERR_BAD_DIRCHAIN);

	strcpy(ctx->ssd_de.name, "__short_sector_data__");
	ctx->ssd_de.type = UCDF_DE_FILE;
	ctx->ssd_de.size = ctx->ssat_len * id_per_sect;
	ctx->ssd_de.is_short = 0;
	ctx->ssd_de.first = ctx->root->first;
	ctx->ssd_de.parent = ctx->ssd_de.next = ctx->ssd_de.children = NULL;

	ctx->ssd_f.ctx = ctx;
	ctx->ssd_f.de = &ctx->ssd_de;
	ctx->ssd_f.stream_offs = ctx->ssd_f.sect_id = ctx->ssd_f.sect_offs = 0;
	return 0;
}

int ucdf_open(ucdf_ctx_t *ctx, const char *path)
{
	ctx->f = fopen(path, "rb");
	if (ctx->f == NULL) {
		ctx->error = UCDF_ERR_OPEN;
		return -1;
	}

	if (ucdf_read_hdr(ctx) != 0)
		goto error;

	if (ucdf_read_sats(ctx) != 0)
		goto error;

	if (ucdf_read_dirs(ctx) != 0)
		goto error;

	if (ucdf_setup_ssd(ctx) != 0)
		goto error;

	return 0;

	error:;
	fclose(ctx->f);
	ctx->f = NULL;
	return -1;
}

static void ucdf_free_dir(ucdf_direntry_t *dir)
{
	ucdf_direntry_t *d, *next;

	for(d = dir->children; d != NULL; d = next) {
		next = d->next;
		ucdf_free_dir(d);
	}
	free(dir);
}

void ucdf_close(ucdf_ctx_t *ctx)
{
	if (ctx->root != NULL) {
		ucdf_free_dir(ctx->root);
		ctx->root = NULL;
	}
	if (ctx->f != NULL) {
		fclose(ctx->f);
		ctx->f = NULL;
	}
	if (ctx->msat != NULL) {
		free(ctx->msat);
		ctx->msat = NULL;
	}
	if (ctx->sat != NULL) {
		free(ctx->sat);
		ctx->sat = NULL;
	}
	if (ctx->ssat != NULL) {
		free(ctx->ssat);
		ctx->ssat = NULL;
	}
}

int ucdf_fopen(ucdf_ctx_t *ctx, ucdf_file_t *fp, ucdf_direntry_t *de)
{
	if (de->type != UCDF_DE_FILE)
		return -1;

	memset(fp, 0, sizeof(ucdf_file_t));
	fp->ctx = ctx;
	fp->de = de;
	fp->sect_id = de->first;

	return 0;
}

long ucdf_fread_short(ucdf_file_t *fp, char *dst, long len)
{
	abort();
}

long ucdf_fread_long(ucdf_file_t *fp, char *dst, long len)
{
	ucdf_ctx_t *ctx = fp->ctx;
	long got = 0;


	while(len > 0) {
		long l, sect_remaining, file_remaining;

		if ((fp->sect_id < 0) || (fp->stream_offs >= fp->de->size))
			break;

		/* read chunk from current sector */
		sect_remaining = ctx->sect_size - fp->sect_offs;
		file_remaining = fp->de->size - fp->stream_offs;
		l = (len < sect_remaining) ? len : sect_remaining;
		l = (l < file_remaining) ? l : file_remaining;
		safe_seek(sect_id2offs(ctx, fp->sect_id) + fp->sect_offs);
		safe_read(dst, l);
		got += l;
		dst += l;
		len -= l;
		fp->sect_offs += l;
		fp->stream_offs += l;

		/* move to next sector if needed */
		if (fp->sect_offs == ctx->sect_size) {
			fp->sect_offs = 0;
			fp->sect_id = ctx->sat[fp->sect_id];
		}
	}

	return got;
}


long ucdf_fread(ucdf_file_t *fp, char *dst, long len)
{
	if (fp->de->is_short) return ucdf_fread_short(fp, dst, len);
	return ucdf_fread_long(fp, dst, len);
}


int ucdf_fseek(ucdf_file_t *fp, long offs)
{
	ucdf_ctx_t *ctx = fp->ctx;
	long sect_min, sect_max;
	long sect_idx, sect_offs, sect_id, n;

	if (fp->de->is_short)
		return -1;

	if (offs == fp->stream_offs)
		return 0; /* no need to move */

	if ((offs < 0) || (offs >= fp->de->size))
		return -1; /* do not seek out of the file */

	/* fast lane: check if seek is within the current sector */
	sect_min = fp->stream_offs - fp->sect_offs;
	sect_max = sect_min + ctx->sect_size;
	if ((offs >= sect_min) && (offs < sect_max)) {
		long delta = offs - fp->stream_offs;
		fp->stream_offs += delta;
		fp->sect_offs += delta;
		return 0;
	}

	/* seeking to another sector; since SAT is a singly linked list, we need
	   to recalculate this sector by sector */
	sect_idx = offs / ctx->sect_size;
	sect_offs = offs % ctx->sect_size;

	sect_id = fp->de->first;
	for(n = 0; n < sect_idx; n++) {
		if (sect_id < 0)
			return -1;
		sect_id = ctx->sat[sect_id];
	}

	if (sect_id < 0)
		return -1;

	fp->stream_offs += offs;
	fp->sect_offs += sect_offs;
	fp->sect_id = sect_id;
	return 0;
}

