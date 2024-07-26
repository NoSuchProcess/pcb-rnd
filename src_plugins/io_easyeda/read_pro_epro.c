#include <genvector/vts0.h>

typedef struct epro_s {
	gds_t zipdir; /* where the zip is unpacked */

	int want_pcb_id;
	char *want_pcb_name;

	vts0_t name_fn; /* odd elems are board names, even elems are corresponding uids (file names) */
} epro_t;


static void epro_load_project_json(epro_t *epro)
{
	
}

static void epro_uninit(epro_t *epro)
{
	TODO("clean up the temp dir");
	gds_uninit(&epro->zipdir);
}

