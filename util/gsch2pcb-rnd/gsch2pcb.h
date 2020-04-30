#include "../src_3rd/genlist/gendlist.h"
#include "../src_3rd/genlist/genadlist.h"

#define TRUE 1
#define FALSE 0

#define GSCH2PCB_RND_VERSION "2.0.0"

#define DEFAULT_PCB_INC "pcb.inc"

#define SEP_STRING "--------\n"

typedef struct {
	char *refdes, *value, *description, *changed_description, *changed_value;
	char *flags, *tail;
	char *x, *y;
	char *pkg_name_fix;
	char res_char;

	gdl_elem_t all_elems;

	unsigned still_exists:1;
	unsigned new_format:1;
	unsigned hi_res_format:1;
	unsigned quoted_flags:1;
	unsigned omit_PKG:1;
	unsigned nonetlist:1;
} PcbElement;

typedef struct {
	char *part_number, *element_name;
} ElementMap;

extern gdl_list_t pcb_element_list;
extern gadl_list_t schematics, extra_gnetlist_arg_list, extra_gnetlist_list;

extern int n_deleted, n_added_ef, n_fixed, n_PKG_removed_new,
           n_PKG_removed_old, n_preserved, n_changed_value, n_not_found,
           n_unknown, n_none, n_empty;

extern int bak_done, need_PKG_purge;

extern const char *local_project_pcb_name; /* file name of the design from which the local project file name shall be derived */

char *fix_spaces(char * str);
void require_gnetlist_backend(const char *dir, const char *backend);

