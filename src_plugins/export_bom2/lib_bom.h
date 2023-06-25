#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <genvector/vts0.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include <genht/ht_utils.h>

/*** formats & templates ***/
typedef struct {
	const char *header, *item, *footer, *subc2id;
	const char *needs_escape; /* list of characters that need escaping */
	const char *escape; /* escape character */
} template_t;

static vts0_t fmt_names; /* array of const char * long name of each format, pointing into the conf database */
static vts0_t fmt_ids;   /* array of strdup'd short name (ID) of each format */

static void free_fmts(void);
static void gather_templates(void);
static void bom_init_template(template_t *templ, const char *tid);



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
	const template_t *templ;
	FILE *f;
} subst_ctx_t;


typedef struct {
	pcb_subc_t *subc; /* one of the subcircuits picked randomly, for the attributes */
	char *id; /* key for sorting */
	gds_t refdes_list;
	long cnt;
} item_t;


/* Export a file; call begin, then loop over all items and call _add, then call
   _all and _end. */
static void bom_print_begin(subst_ctx_t *ctx, FILE *f, const template_t *templ); /* init ctx, print header */
static void bom_print_add(subst_ctx_t *ctx, pcb_subc_t *subc, const char *name); /* add an app_item */
static void bom_print_all(subst_ctx_t *ctx); /* sort and print all items */
static void bom_print_end(subst_ctx_t *ctx); /* print footer and uninit ctx */
