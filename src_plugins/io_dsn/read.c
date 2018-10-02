/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format read, parser
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <gensexpr/gsxl.h>
#include <ctype.h>

#include "plug_io.h"
#include "error.h"
#include "pcb_bool.h"
#include "safe_fs.h"
#include "compat_misc.h"

#include "read.h"

typedef struct {
	gsxl_dom_t dom;
	const pcb_unit_t *unit;
} dsn_read_t;

#define if_save_uniq(node, name) \
	if (pcb_strcasecmp(node->str, #name) == 0) { \
		if (n ## name != NULL) { \
			pcb_message(PCB_MSG_ERROR, "Multiple " #name " nodes where only one is expected (at %ld:%ld)\n", (long)node->line, (long)node->col); \
			return -1; \
		} \
		n ## name = node; \
	}

static const pcb_unit_t *push_unit(dsn_read_t *ctx, gsxl_node_t *nu)
{
	const pcb_unit_t *old = ctx->unit;

	if ((nu == NULL) || (nu->children == NULL))
		return ctx->unit;
	
	ctx->unit = get_unit_struct(nu->children->str);
	if (ctx->unit == NULL) {
		pcb_message(PCB_MSG_ERROR, "Invalid unit: '%s' (at %ld:%ld)\n", nu->children->str, (long)nu->line, (long)nu->col);
		return NULL;
	}

	return old;
}

static void pop_unit(dsn_read_t *ctx, const pcb_unit_t *saved)
{
	ctx->unit = saved;
}

/*** tree parse ***/
static int dsn_parse_pcb(dsn_read_t *ctx, gsxl_node_t *root)
{
	gsxl_node_t *n, *nunit = NULL, *nstruct = NULL, *nplacement = NULL, *nlibrary = NULL, *nnetwork = NULL, *nwiring = NULL, *ncolors = NULL, *nresolution = NULL;

	/* default unit in case the file does not specify one */
	ctx->unit = get_unit_struct("inch");

	for(n = root->children->next; n != NULL; n = n->next) {
		if (n->str == NULL)
			continue;
		else if_save_uniq(n, unit)
		else if_save_uniq(n, resolution)
		else if_save_uniq(n, struct)
		else if_save_uniq(n, placement)
		else if_save_uniq(n, library)
		else if_save_uniq(n, network)
		else if_save_uniq(n, wiring)
		else if_save_uniq(n, colors)
	}

	if ((nresolution != NULL) && (nresolution->children != NULL)) {
		ctx->unit = get_unit_struct(nresolution->children->str);
		if (ctx->unit == NULL) {
			pcb_message(PCB_MSG_ERROR, "Invalid resolution unit: '%s'\n", nresolution->children->str);
			return -1;
		}
	}

	if (push_unit(ctx, nunit) == NULL)
		return -1;

	return 0;
}

/*** glue and low level ***/


int io_dsn_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;
	int phc = 0, in_pcb = 0, lineno = 0;

	if (typ != PCB_IOT_PCB)
		return 0;

	while(!(feof(f)) && (lineno < 512)) {
		if (fgets(line, sizeof(line), f) != NULL) {
			lineno++;
			for(s = line; *s != '\0'; s++)
				if (*s == '(')
					phc++;
			s = line;
			if ((phc > 0) && (strstr(s, "pcb") != 0))
				in_pcb = 1;
			if ((phc > 2) && in_pcb && (strstr(s, "space_in_quoted_tokens") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_cad") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_version") != 0))
				return 1;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}

static int dsn_parse_file(dsn_read_t *rdctx, const char *fn)
{
	int c, blen = -1;
	gsx_parse_res_t res;
	FILE *f;
	char buff[12];
	long q_offs = -1, offs;


	f = pcb_fopen(fn, "r");
	if (f == NULL)
		return -1;

	/* find the offset of the quote char so it can be ignored during the s-expression parse */
	offs = 0;
	while(!(feof(f))) {
		c = fgetc(f);
		if (c == 's')
			blen = 0;
		if (blen >= 0) {
			buff[blen] = c;
			if (blen == 12) {
				if (memcmp(buff, "string_quote", 12) == 0) {
					for(c = fgetc(f),offs++; isspace(c); c = fgetc(f),offs++) ;
					q_offs = offs;
					printf("quote is %c at %ld\n", c, q_offs);
					break;
				}
				blen = -1;
			}
			blen++;
		}
		offs++;
	}
	rewind(f);


	gsxl_init(&rdctx->dom, gsxl_node_t);
	rdctx->dom.parse.line_comment_char = '#';
	offs = 0;
	do {
		c = fgetc(f);
		if (offs == q_offs) /* need to ignore the quote char else it's an unbalanced quote */
			c = '.';
		offs++;
	} while((res = gsxl_parse_char(&rdctx->dom, c)) == GSX_RES_NEXT);

	fclose(f);
	if (res != GSX_RES_EOE)
		return -1;

	return 0;
}

int io_dsn_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	dsn_read_t rdctx;
	gsxl_node_t *rn;
	int ret;

	memset(&rdctx, 0, sizeof(rdctx));
	if (dsn_parse_file(&rdctx, Filename) != 0) {
		pcb_message(PCB_MSG_ERROR, "s-expression parse error\n");
		goto error;
	}

	gsxl_compact_tree(&rdctx.dom);
	rn = rdctx.dom.root;
	if (pcb_strcasecmp(rn->str, "pcb") != 0) {
		pcb_message(PCB_MSG_ERROR, "Root node should be pcb, got %s instead\n", rn->str);
		goto error;
	}
	if (gsxl_next(rn) != NULL) {
		pcb_message(PCB_MSG_ERROR, "Multiple root nodes?!\n");
		goto error;
	}

	ret = dsn_parse_pcb(&rdctx, rn);
	gsxl_uninit(&rdctx.dom);
	return ret;

	error:;
	gsxl_uninit(&rdctx.dom);
	return -1;
}

