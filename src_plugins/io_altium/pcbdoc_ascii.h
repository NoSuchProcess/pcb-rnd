#include "plug_io.h"
#include <genlist/gendlist.h>
#include <librnd/core/hidlib.h>
#include "altium_kw_sphash.h"

typedef struct altium_field_s {
	altium_kw_field_keys_t type; /* derived from ->key */
	const char *key;
	enum { ALTIUM_FT_STR, ALTIUM_FT_CRD, ALTIUM_FT_DBL, ALTIUM_FT_LNG } val_type;
	union {
		const char *str;
		rnd_coord_t crd;
		double dbl;
		long lng;                  /* also used for bool */
	} val;
	gdl_elem_t link;             /* in parent record */
} altium_field_t;

typedef struct altium_record_s {
	altium_kw_record_keys_t type; /* derived from ->type_s */
	const char *type_s;
	gdl_list_t fields;
	gdl_elem_t link;              /* in tree records */
} altium_record_t;

typedef struct altium_block_s {
	gdl_elem_t link;              /* in tree blocks */
	long size;                    /* allocated size of raw */
	char raw[1];                  /* bytes read from the file */
} altium_block_t;

typedef struct altium_tree_s {
	gdl_list_t rec[altium_kw_record_SPHASH_MAXVAL+1]; /* ordered list of records per type */
	gdl_list_t blocks;
} altium_tree_t;

int pcbdoc_ascii_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *file_name, FILE *f);
int pcbdoc_ascii_parse_file(rnd_design_t *hidlib, altium_tree_t *tree, const char *fn);

#define altium_kw_AUTO (-2)
altium_record_t *pcbdoc_ascii_new_rec(altium_tree_t *tree, const char *type_s, int type);
altium_field_t *pcbdoc_ascii_new_field(altium_tree_t *tree, altium_record_t *rec, const char *key, int kw, const char *val);

/* Parse '|' separated fields and allocate new altium_field_t's for them and
   put them under rec. Return 0 on success. Stop at \n, 'fields' points to
   the next char after the \n */
int pcbdoc_ascii_parse_fields(altium_tree_t *tree, altium_record_t *rec, const char *fn, long line, char **fields);


void altium_tree_free(altium_tree_t *tree);

