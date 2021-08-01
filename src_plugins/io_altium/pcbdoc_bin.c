/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  altium plugin - pcbdoc low level read: parse binary into a tree in mem
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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


#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libucdf/ucdf.h>
#include <librnd/core/compat_misc.h>
#include "pcbdoc_ascii.h"
#include "pcbdoc_bin.h"
#include "altium_kw_sphash.h"

TODO("remove:")
#include <stdio.h>

/* optional trace */
#if 1
#	define tprintf printf
#else
	static int tprintf(const char *fmt, ...) { return 0; }
#endif

#define MAX_REC_LEN (1UL << 20)

static const union {
	short int sh;
	char c[2];
} endianness = {0x0102};

#define is_host_litend (endianness.c[0] == 2)

/* convert a file integer (len bytes wide) into a native integer, assuming source is little-endian */
static long load_int(unsigned const char *src, int len)
{
	int n, sh;
	long tmp, res = 0;

	for(sh = n = 0; n < len; n++,sh += 8,src++) {
		tmp = *src;
		tmp = (tmp << sh);
		res |= tmp;
	}

	/* "sign extend" */
	if (res >= (1UL << (len*8-1)))
		res = -((1UL << (len*8)) - res);

	return res;
}

/* convert a double, assuming source is little-endian */
static double load_dbl(unsigned const char *src)
{
	double d;

	if (is_host_litend) { /* le to le -> copy */
		memcpy(&d, src, 8);
		return d;
	}
	else { /* le to be -> swap */
		unsigned char tmp[8];
		tmp[0] = src[7]; tmp[1] = src[6]; tmp[2] = src[5]; tmp[3] = src[4];
		tmp[4] = src[3]; tmp[5] = src[2]; tmp[6] = src[1]; tmp[7] = src[0];
		memcpy(&d, tmp, 8);
		return d;
	}
}

static double bmil(unsigned const char *src)
{
	long raw = load_int(src, 4);
	return (double)raw / 10000.0;
}

#define buf_grow(tmp, len) \
do { \
	if (tmp->alloced < len) { \
		free(tmp->data); \
		tmp->data = malloc(len); \
		if (tmp->data == NULL) { \
			tmp->alloced = 0; \
			return -1; \
		} \
		tmp->alloced = len; \
	} \
} while(0)

static long read_len(ucdf_file_t *fp, int field_width)
{
	unsigned char lens[4];
	long len, res;

	assert(field_width <= 4);

	if ((res = ucdf_fread(fp, (void *)lens, field_width)) != field_width) {
		if (res <= 0)
			return res;
		return -1; /* short read means broken field */
	}

	len = load_int(lens, field_width);
	if ((len < 0) || (len >= MAX_REC_LEN))
		return -1;

	return len;
}

static long read_rec_l4b(ucdf_file_t *fp, altium_buf_t *tmp)
{
	long len, res;

	len = read_len(fp, 4);
	if (len < 0)
		return len;

	buf_grow(tmp, len+1);
	res = ucdf_fread(fp, (void *)tmp->data, len);
	if (res == 0)
		return 0;

	if (res != len)
		return -1;

	tmp->data[len] = '\0'; /* safety: always make it a valid string just in case it is text */
	tmp->used = len+1;

	return len;
}

static long read_rec_tlb(ucdf_file_t *fp, altium_buf_t *tmp, int *type)
{
	char typec;
	long res;

	if ((res = ucdf_fread(fp, (void *)&typec, 1)) != 1)
		return res;

	*type = typec;

	return read_rec_l4b(fp, tmp);
}

static long read_rec_ilb(ucdf_file_t *fp, altium_buf_t *tmp, int *id)
{
	unsigned char ids[2];
	long res;

	if ((res = ucdf_fread(fp, (void *)ids, 2)) != 2)
		return res;

	*id = load_int(ids, 2);

	return read_rec_l4b(fp, tmp);
}

static int pcbdoc_bin_parse_ascii(rnd_hidlib_t *hidlib, altium_tree_t *tree, const char *record, int kw, altium_buf_t *tmp, long len)
{
	altium_block_t *blk;
	altium_record_t *rec;
	char *curr, *end;
	int res = 0;

	if (len == 0)
		return 0;

	/* allocate a persistent block and copy raw data from tmp */
	blk = malloc(sizeof(altium_block_t) + len + 1);
	if (blk == NULL) {
		fprintf(stderr, "pcbdoc_bin_parse_ascii: failed to alloc memory\n");
		return -1;
	}
	memset(&blk->link, 0, sizeof(blk->link));
	blk->size = len;
	memcpy(blk->raw, tmp->data, len);
	gdl_append(&tree->blocks, blk, link);

	/* need to terminate text data with a \n to emulate the text file for the ascii parser */
	end = blk->raw + len;
	end[-1] = '\n';
	end[0] = '\0';


	/* create the record and parse fields under it */
	rec = pcbdoc_ascii_new_rec(tree, record, kw);
	for(curr = blk->raw; curr < end;) {
		while((*curr == '\r') || (*curr == '\n') || (*curr == '|')) curr++;
		res |= pcbdoc_ascii_parse_fields(tree, rec, "<binary>", 1, &curr);
	}

	return res;
}

static int pcbdoc_bin_parse_any_ascii(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp, const char *recname, int kw)
{
	for(;;) {
		long len = read_rec_l4b(fp, tmp);
		if (len <= 0)
			return len;
		if (pcbdoc_bin_parse_ascii(hidlib, tree, recname, kw, tmp, len) != 0)
			return -1;
	}
	return 0;
}

#define FIELD_STR(rec, key, val_str) \
	pcbdoc_ascii_new_field(tree, rec, #key, altium_kw_field_ ## key, val_str)

#define FIELD_CRD(rec, key, val_mil) \
	do { \
		altium_field_t *fld = FIELD_STR(rec, key, NULL); \
		fld->val.crd = RND_MIL_TO_COORD(val_mil); \
		fld->val_type = ALTIUM_FT_CRD; \
	} while(0)

#define FIELD_DBL(rec, key, val_dbl) \
	do { \
		altium_field_t *fld = FIELD_STR(rec, key, NULL); \
		fld->val.dbl = val_dbl; \
		fld->val_type = ALTIUM_FT_DBL; \
	} while(0)

#define FIELD_LNG(rec, key, val_lng) \
	do { \
		altium_field_t *fld = FIELD_STR(rec, key, NULL); \
		fld->val.lng = val_lng; \
		fld->val_type = ALTIUM_FT_LNG; \
	} while(0)

/*** file/field parsers ***/

int pcbdoc_bin_parse_board6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	return pcbdoc_bin_parse_any_ascii(hidlib, tree, fp, tmp, "Board", altium_kw_record_board);
}

int pcbdoc_bin_parse_polygons6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	return pcbdoc_bin_parse_any_ascii(hidlib, tree, fp, tmp, "Polygon", altium_kw_record_polygon);
}

int pcbdoc_bin_parse_classes6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	return pcbdoc_bin_parse_any_ascii(hidlib, tree, fp, tmp, "Class", altium_kw_record_class);
}

int pcbdoc_bin_parse_nets6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	return pcbdoc_bin_parse_any_ascii(hidlib, tree, fp, tmp, "Net", altium_kw_record_net);
}

int pcbdoc_bin_parse_components6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	return pcbdoc_bin_parse_any_ascii(hidlib, tree, fp, tmp, "Component", altium_kw_record_component);
}

int pcbdoc_bin_parse_rules6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		int id;
		long len = read_rec_ilb(fp, tmp, &id);
		if (len <= 0)
			return len;
		TODO("Do we need the id?");
		printf("id=%d\n", id);
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Rule", altium_kw_record_rule, tmp, len) != 0)
			return -1;
	}
	return 0;
}


int pcbdoc_bin_parse_tracks6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;
		altium_record_t *rec;
		
		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 4) {
			rnd_message(RND_MSG_ERROR, "Tracks6 record with wrong type; expected 4, got %d\n", rectype);
			continue;
		}
		if (len < 45) {
			rnd_message(RND_MSG_ERROR, "Tracks6 record too short; expected 45, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("line: layer=%d ko=%d net=%ld poly=%ld comp=%ld width=%.2f uu=%d%d\n",
		d[0], d[1], load_int(d+3, 2), load_int(d+5, 2), load_int(d+7, 2), bmil(d+29), d[36], d[40]);
		printf("  x1=%.2f y1=%.2f x2=%.2f y2=%.2f\n", bmil(d+13), bmil(d+17), bmil(d+21), bmil(d+25));


		rec = pcbdoc_ascii_new_rec(tree, "Track", altium_kw_record_track);
		FIELD_LNG(rec, layer, d[0]);
		FIELD_CRD(rec, x1, bmil(d+13));
		FIELD_CRD(rec, y1, bmil(d+17));
		FIELD_CRD(rec, x2, bmil(d+21));
		FIELD_CRD(rec, y2, bmil(d+25));
		FIELD_CRD(rec, width, bmil(d+29));
	}
	return 0;
}


int pcbdoc_bin_parse_arcs6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;
		
		
		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 1) {
			rnd_message(RND_MSG_ERROR, "Arcs6 record with wrong type; expected 1, got %d\n", rectype);
			continue;
		}
		if (len < 56) {
			rnd_message(RND_MSG_ERROR, "Arcs6 record too short; expected 56, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("arc: layer=%d ko=%d net=%ld poly=%ld comp=%ld width=%.2f uu=%d%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+5, 2), load_int(d+7, 2), bmil(d+41), d[48], d[55]);
		printf("  cx=%.2f cy=%.2f r=%.2f sa=%.3f ea=%.3f\n", bmil(d+13), bmil(d+17), bmil(d+21), load_dbl(d+25), load_dbl(d+33));
	}
	return 0;
}


int pcbdoc_bin_parse_texts6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;

		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 5) {
			rnd_message(RND_MSG_ERROR, "Texts6 record with wrong type; expected 5, got %d\n", rectype);
			continue;
		}
		if (len < 230) {
			rnd_message(RND_MSG_ERROR, "Texts6 record too short; expected 230, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("text: layer=%d comp=%ld height=%.2f comment=%d designator=%d\n", d[0], load_int(d+7, 2), bmil(d+21), d[40], d[41]);
		printf("  x=%.2f y=%.2f h=%.2f rot=%.3f mirr=%d\n", bmil(d+13), bmil(d+17), bmil(d+21), load_dbl(d+27), d[35]);

		len = read_rec_l4b(fp, tmp);
		printf("  string='%s'\n", tmp->data+1);

	}
	return 0;
}


int pcbdoc_bin_parse_fills6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;

		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 6) {
			rnd_message(RND_MSG_ERROR, "Fills6 record with wrong type; expected 6, got %d\n", rectype);
			continue;
		}
		if (len < 46) {
			rnd_message(RND_MSG_ERROR, "Fills6 record too short; expected 46, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("fill: layer=%d ko=%d net=%ld comp=%ld u=%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+7, 2), d[45]);
		printf("  x1=%.2f y1=%.2f x2=%.2f y2=%.2f rot=%.3f dir2=%.3f\n", bmil(d+13), bmil(d+17), bmil(d+21), bmil(d+25), load_dbl(d+29), load_dbl(d+38));

	}
	return 0;
}


int pcbdoc_bin_parse_vias6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;

		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 3) {
			rnd_message(RND_MSG_ERROR, "vias6 record with wrong type; expected 3, got %d\n", rectype);
			continue;
		}
		if (len < 209) {
			rnd_message(RND_MSG_ERROR, "vias6 record too short; expected 209, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("via: layer=%d..%d net=%ld (comp=%ld) u=%d C*=%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+7, 2), d[70], d[40]);
		printf("  x=%.2f y=%.2f copperD=%.2f holD=%.2f\n", bmil(d+13), bmil(d+17), bmil(d+21), bmil(d+25));
	}
	return 0;
}

/* parse the fields of a pad (the 120 long block of pads6) */
static int pcbdoc_bin_parse_pads6_fields(rnd_hidlib_t *hidlib, altium_tree_t *tree, altium_buf_t *tmp, const char *name)
{
	unsigned char *d = tmp->data;

	/* if layer[0] is 74 (multilayer), use all shapes, else use only top on the specified layer */

	printf("pad: layer=%d..%d net=%ld (comp=%ld) '%s'\n", d[0], d[1], load_int(d+3, 2), load_int(d+7, 2), name);
	printf("  x=%.2f y=%.2f hole=%.2f plated=%d mode=%d rot=%.3f\n", bmil(d+13), bmil(d+17), bmil(d+45), d[60], d[62], load_dbl(d+52));
	printf("  top: %d sx=%.2f sy=%.2f; mid: %d sx=%.2f sy=%.2f; bot: %d sx=%.2f mid-sy=%.2f\n", d[49], bmil(d+21), bmil(d+25), d[50], bmil(d+29), bmil(d+33), d[51], bmil(d+37), bmil(d+41));
	return 0;
}

int pcbdoc_bin_parse_pads6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		char rtype;
		long res;

		if ((res = ucdf_fread(fp, (void *)&rtype, 1)) != 1)
			return res;

		if (rtype == 2) {
			long len;
			char name[260];
			int n;

			len = read_rec_l4b(fp, tmp);
			if (len <= 0)
				return len;

			/* 0: tmp is name */
			if (len >= 256) {
				rnd_message(RND_MSG_ERROR, "pads6 name too long: %ld (truncating)\n", len);
				len = 256;
			}
			tmp->data[len] = '\0';
			strcpy(name, (char *)tmp->data+1);

			for(n = 0; ; n++) {
				len = read_rec_l4b(fp, tmp);
/*				printf("len[%d]=%ld\n", n, len);*/
				if (len == 0)
					break;
				if (len < 0)
					return len;
/*				if ((n == 0)||(n == 2)) { printf("#%d: %d (len=%ld)\n", n, tmp->data[0], len); }*/
/*				if (n == 1) { printf("#%d: %d %d %d %d %d\n", n, tmp->data[0], tmp->data[1], tmp->data[2], tmp->data[3], tmp->data[4]); }*/
				if (n == 3) {
					if (len < 120)
						rnd_message(RND_MSG_ERROR, "pads6 record block3 with wrong lengt; expected 120, got %ld\n", len);
					else if (pcbdoc_bin_parse_pads6_fields(hidlib, tree, tmp, name) != 0)
						return -1;
				}
			}
/*			printf("tell=%ld %x\n", ftell(fp), ftell(fp));*/
		}
		else { /* not a pad */
			long len;

			fprintf(stderr, "non-pad object in padstack!\n");
			len = read_rec_l4b(fp, tmp);
			if (len <= 0)
				return len;

			TODO("read and handle snippet of slen in tmp");
			switch(rtype) {
				case 1: /* arc */
				case 4: /* track */
				case 6: /* fill */
					break;
			}
		}

	}
	return 0;
}

int pcbdoc_bin_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *file_name, FILE *f)
{
	return (ucdf_test_parse(file_name) == 0);
}

/* Look up the Data child of de and return it if it is a file; return NULL
   on error */
static ucdf_direntry_t *get_data_de(ucdf_direntry_t *de)
{
	ucdf_direntry_t *ded;

	for(ded = de->children; ded != NULL; ded = ded->next) {
		if (rnd_strcasecmp(ded->name, "Data") == 0) {
			if (ded->type == UCDF_DE_FILE)
				return ded;
			return NULL;
		}
	}

	return NULL;
}

/* Take the Data child of de, open it and parse it using 'call' */
#define load_stream(de, call) \
	do { \
		ded = get_data_de(de); \
		if (ded == NULL) {\
			tprintf(" bin parse: " #call ": no data\n"); \
			break; \
		} \
		res = ucdf_fopen(&uctx, &fp, ded); \
		if (res != 0) {\
			tprintf(" bin parse: " #call ": failed to open\n"); \
			break; \
		} \
		tprintf(" bin parse: "  #call "\n"); \
		res = call(hidlib, tree, &fp, &tmp); \
		if (res != 0) { \
			rnd_message(RND_MSG_ERROR, "(PcbDoc bin: abort parsing due to errro in " #call ")\n"); \
			goto error; \
		} \
	} while(0)

int pcbdoc_bin_parse_file(rnd_hidlib_t *hidlib, altium_tree_t *tree, const char *fn)
{
	int res;
	ucdf_ctx_t uctx = {0};
	ucdf_file_t fp;
	ucdf_direntry_t *de, *ded;
	altium_buf_t tmp = {0};

	res = ucdf_open(&uctx, fn);
	tprintf("ucdf open: %d\n", res);
	if (res != 0)
		return -1;

	/* look at each main directory and see if we can parse them */
	for(de = uctx.root->children; de != NULL; de = de->next) {
		int kw = altium_kw_sphash(de->name);
		switch(kw) {
			case altium_kw_bin_names_arcs6:           load_stream(de, pcbdoc_bin_parse_arcs6); break;
			case altium_kw_bin_names_board6:          load_stream(de, pcbdoc_bin_parse_board6); break;
			case altium_kw_bin_names_classes6:        load_stream(de, pcbdoc_bin_parse_classes6); break;
			case altium_kw_bin_names_components6:     load_stream(de, pcbdoc_bin_parse_components6); break;
			case altium_kw_bin_names_fills6:          load_stream(de, pcbdoc_bin_parse_fills6); break;
			case altium_kw_bin_names_nets6:           load_stream(de, pcbdoc_bin_parse_nets6); break;
			case altium_kw_bin_names_pads6:           load_stream(de, pcbdoc_bin_parse_pads6); break;
			case altium_kw_bin_names_polygons6:       load_stream(de, pcbdoc_bin_parse_polygons6); break;
			case altium_kw_bin_names_rules6:          load_stream(de, pcbdoc_bin_parse_rules6); break;
			case altium_kw_bin_names_texts6:          load_stream(de, pcbdoc_bin_parse_texts6); break;
			case altium_kw_bin_names_tracks6:         load_stream(de, pcbdoc_bin_parse_tracks6); break;
			case altium_kw_bin_names_vias6:           load_stream(de, pcbdoc_bin_parse_vias6); break;
		}
	}

	ucdf_close(&uctx);
	return 0;
	error:;
	tprintf("-> failed to parse binary PcbDoc\n");
	ucdf_close(&uctx);
	return -1;
}

