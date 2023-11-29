/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <genvector/vts0.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include <genht/ht_utils.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>

/*** formats & templates ***/
typedef struct bom_template_s {
	const char *header, *item, *footer, *sort_id;
	const char *needs_escape; /* list of characters that need escaping */
	const char *escape; /* escape character */
	const char *skip_if_empty; /* if this template is not empty: render the template for each item and do not include the item if the rendered string is empty */
	const char *skip_if_nonempty; /* if this template is not empty: render the template for each item and do not include the item if the rendered string is not empty */
	const char *list_sep; /* separator sequence used when building a list */
} bom_template_t;

static vts0_t bom_fmt_names; /* array of const char * long name of each format, pointing into the conf database */
static vts0_t bom_fmt_ids;   /* array of strdup'd short name (ID) of each format */

/* Call these once on plugin init/uninit */
static void bom_fmt_init(void);
static void bom_fmt_uninit(void);

/* (re)build bom_fmt_* arrays from the config list; call this before
   initializing plugin format options enum */
static void bom_build_fmts(const rnd_conflist_t *templates);

/* Choose one of the templates by name (tid) from the config list and
   load fields of templ with the relevant template strings; call this
   before starting an export for a specific format */
static void bom_init_template(bom_template_t *templ, const rnd_conflist_t *templates, const char *tid);


/*** subst ***/

/* The caller needs to typedef bom_obj_t to the app-specific object type that
   is used as input for bom listings */


typedef struct {
	char utcTime[64];
	char *name;
	bom_obj_t *obj;
	long count;
	gds_t tmp;
	const char *needs_escape; /* list of characters that need escaping */
	const char *escape; /* escape character or NULL for replacing with _*/

	/* print/sort state */
	htsp_t tbl;
	vtp0_t arr;
	const bom_template_t *templ;
	FILE *f;

	/* separator sequence used when printing lists */
	const char *list_sep;
} bom_subst_ctx_t;

/* Export a file; call begin, then loop over all items and call _add, then call
   _all and _end. */
static void bom_print_begin(bom_subst_ctx_t *ctx, FILE *f, const bom_template_t *templ); /* init ctx, print header */
static void bom_print_add(bom_subst_ctx_t *ctx, bom_obj_t *obj, const char *name); /* add an app_item */
static void bom_print_all(bom_subst_ctx_t *ctx); /* sort and print all items */
static void bom_print_end(bom_subst_ctx_t *ctx); /* print footer and uninit ctx */
