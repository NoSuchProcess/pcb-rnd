/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  altium pcbdoc ASCII plugin - low level read: parse into a tree in mem
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

#include <stdio.h>
#include <string.h>

#include "pcbdoc_ascii.h"

#define block_size 65536L

/* optional trace */
#if 0
#	define tprintf printf
#else
	static int tprintf(const char *fmt, ...) { return 0; }
#endif

int pcbdoc_ascii_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *file_name, FILE *f)
{
	char line[256], *s;

	/* first line should comply to the low level file format */
	fgets(line, sizeof(line), f);

	s = line;
	if (*s == '|') s++;

	/* every line must start with a RECORD field, the first one too */
	if (strncmp(s,"RECORD=", 7) != 0)
		return 0;

	/* there must be a field separator */
	if (strchr(s, '|') == NULL)
		return 0;

	return 1;
}

static int pcbdoc_ascii_load_blocks(altium_tree_t *tree, FILE *f, long max)
{
	long curr = 0, next;

	for(;;) {
		int c;
		altium_block_t *blk;

		/* seek at potential end-of-block */
		next = curr + block_size;
		if (next > max-1)
			next = max-1;
		fseek(f, next, SEEK_SET);

		/* seek first record separator (or eof) */
		for(;;) {
			c = fgetc(f);
			if (c == EOF)
				break;
			next++;
			if ((c == '\r') || (c == '\n'))
				break;
		}

		if (c != EOF) {
			/* seek end of record separator section (typica: \r\n) */
			for(;;) {
				c = fgetc(f);
				if (c == EOF)
					break;
				if ((c != '\r') && (c != '\n'))
					break;
				next++;
			}
		}

		if (next == curr)
			break;

		/* by now "next" points to the first character of the next block */
		blk = malloc(sizeof(altium_block_t) + (next-curr) + 1);
		if (blk == NULL) {
			fprintf(stderr, "pcbdoc_ascii_load_blocks: failed to alloc memory\n");
			return -1;
		}
		memset(&blk->link, 0, sizeof(blk->link));
		blk->size = next-curr;
		fseek(f, curr, SEEK_SET);
		if (fread(&blk->raw, blk->size, 1, f) != 1) {
			free(blk);
			fprintf(stderr, "pcbdoc_ascii_load_blocks: can't read that many: %ld from %ld (%ld; max is %ld)\n", blk->size, curr, curr+blk->size, max);
			return -1;
		}
		blk->raw[blk->size] = '\0';
		gdl_append(&tree->blocks, blk, link);
/*		printf("curr=%ld next=%ld\n", curr, next);*/
		curr = next;
	}

	return 0;
}


TODO("these two 'new' functions should use stack-slabs from umalloc")
static altium_record_t *pcbdoc_ascii_new_rec(altium_tree_t *tree, const char *type_s)
{
	altium_record_t *rec = calloc(sizeof(altium_record_t), 1);
	int kw = altium_kw_sphash(type_s);

	if ((kw < altium_kw_record_SPHASH_MINVAL) || (kw > altium_kw_record_SPHASH_MAXVAL))
		kw = altium_kw_record_misc;

	rec->type = kw;
	rec->type_s = type_s;

	gdl_append(&tree->rec[kw], rec, link);

	return rec;
}

static altium_field_t *pcbdoc_ascii_new_field(altium_tree_t *tree, altium_record_t *rec, const char *key, const char *val)
{
	altium_field_t *field = calloc(sizeof(altium_field_t), 1);
	int kw = altium_kw_sphash(key);

	if ((kw < altium_kw_field_SPHASH_MINVAL) || (kw > altium_kw_field_SPHASH_MAXVAL))
		kw = altium_kw_record_SPHASH_INVALID;

	field->type = kw;
	field->key  = key;
	field->val  = val;

	gdl_append(&rec->fields, field, link);

	return field;
}

static int pcbdoc_ascii_parse_blocks(altium_tree_t *tree, const char *fn)
{
	altium_block_t *blk;
	long line = 1;

	for(blk = gdl_first(&tree->blocks); blk != NULL; blk = gdl_next(&tree->blocks, blk)) {
		char *s = blk->raw, *end;

		tprintf("---blk---\n");

		for(;;) { /*  parse each line within the block */
			altium_record_t *rec;
			int nl = 0;

			/* ignore leading seps and newlines, exit if ran out of the string */
			while((*s == '|') || (*s == '\r') || (*s == '\n')) s++;
			if (*s == '\0')
				break;

			/* parse the record header */
			if (strncmp(s, "RECORD=", 7) != 0) {
				fprintf(stderr, "First field must be record in %s:%ld\n", fn, line);
				return -1;
			}
			s+=7;
			end = strpbrk(s, "|\r\n");
			if (end == NULL) {
				fprintf(stderr, "Unterminated record in %s:%ld\n", fn, line);
				return -1;
			}
			*end = '\0';
			tprintf("rec='%s'\n", s);
			rec = pcbdoc_ascii_new_rec(tree, s);
			s = end+1;


			/* parse fields */
			for(;;) {
				char *key, *val;

				/* ignore leading seps and newlines, exit if ran out of the string */
				while(*s == '|') s++;
				if (*s == '\0')
					break;

				/* find sep */
				end = strpbrk(s, "|\r\n");
				if (end == NULL) {
					fprintf(stderr, "Unterminated field in %s:%ld\n", fn, line);
					return -1;
				}
				if (*end != '|')
					nl = 1;
				*end = '\0';

				key = s;
				val = strchr(s, '=');
				if (val != NULL) {
					*val = '\0';
					val++;
				}
				else
					val = end;
				
				tprintf("  %s=%s\n", key, val);
				pcbdoc_ascii_new_field(tree, rec, key, val);
				s = end+1;
				if (nl)
					break;
			}
		}
	}

	return 0;
}

int pcbdoc_ascii_parse_file(rnd_hidlib_t *hidlib, altium_tree_t *tree, const char *fn)
{
	FILE *f;
	int res;
	long filesize;

	filesize = rnd_file_size(hidlib, fn);
	if (filesize <= 0)
		return -1;

	f = fopen(fn, "rb");
	if (f == NULL)
		return -1;

	res = pcbdoc_ascii_load_blocks(tree, f, filesize);
	fclose(f);

	if (res != 0)
		return -1;


	return pcbdoc_ascii_parse_blocks(tree, fn);
}


void altium_tree_free(altium_tree_t *tree)
{
	altium_block_t *blk;
	altium_record_t *rec;
	int n;

	for(blk = gdl_first(&tree->blocks); blk != NULL; blk = gdl_first(&tree->blocks)) {
		gdl_remove(&tree->blocks, blk, link);
		free(blk);
	}

	for(n = 0; n < altium_kw_record_SPHASH_MAXVAL+1; n++) {
		for(rec = gdl_first(&tree->rec[n]); rec != NULL; rec = gdl_first(&tree->rec[n])) {
			altium_field_t *field;

			for(field = gdl_first(&rec->fields); field != NULL; field = gdl_first(&rec->fields)) {
				gdl_remove(&rec->fields, field, link);
				free(field);
			}
			gdl_remove(&tree->rec[n], rec, link);
			free(rec);
		}
	}
}

