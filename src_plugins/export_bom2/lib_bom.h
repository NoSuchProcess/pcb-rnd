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
} subst_ctx_t;


typedef struct {
	pcb_subc_t *subc; /* one of the subcircuits picked randomly, for the attributes */
	char *id; /* key for sorting */
	gds_t refdes_list;
	long cnt;
} item_t;


static void fprintf_templ(FILE *f, subst_ctx_t *ctx, const char *templ);
static char *render_templ(subst_ctx_t *ctx, const char *templ);

/* temporary */
static int item_cmp(const void *item1, const void *item2);
