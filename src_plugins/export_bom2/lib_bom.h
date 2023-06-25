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
} bom_template_t;

static vts0_t bom_fmt_names; /* array of const char * long name of each format, pointing into the conf database */
static vts0_t bom_fmt_ids;   /* array of strdup'd short name (ID) of each format */

/* Call these once on plugin init/uninit */
static void bom_fmt_init(void);
static void bom_fmt_uninit(void);

static void bom_build_fmts(const rnd_conflist_t *templates);

static void bom_gather_templates(const rnd_conflist_t *templates);

static void bom_init_template(bom_template_t *templ, const rnd_conflist_t *templates, const char *tid);


/*** subst ***/
typedef struct {
	char utcTime[64];
	char *name;
	pcb_subc_t *subc;
	long count;
	gds_t tmp;
	const char *needs_escape; /* list of characters that need escaping */
	const char *escape; /* escape character or NULL for replacing with _*/

	/* print/sort state */
	htsp_t tbl;
	vtp0_t arr;
	const bom_template_t *templ;
	FILE *f;
} bom_subst_ctx_t;

/* Export a file; call begin, then loop over all items and call _add, then call
   _all and _end. */
static void bom_print_begin(bom_subst_ctx_t *ctx, FILE *f, const bom_template_t *templ); /* init ctx, print header */
static void bom_print_add(bom_subst_ctx_t *ctx, pcb_subc_t *subc, const char *name); /* add an app_item */
static void bom_print_all(bom_subst_ctx_t *ctx); /* sort and print all items */
static void bom_print_end(bom_subst_ctx_t *ctx); /* print footer and uninit ctx */
