#include "plug_io.h"
#include <genlist/gendlist.h>
#include <librnd/core/hidlib.h>
#include "altium_kw_sphash.h"

typedef struct altium_field_s {
	altium_kw_field_keys_t type; /* derived from ->key */
	const char *key;
	const char *val;
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
int pcbdoc_ascii_parse_file(rnd_hidlib_t *hidlib, altium_tree_t *tree, const char *fn);

#define altium_kw_AUTO (-2)


void altium_tree_free(altium_tree_t *tree);

