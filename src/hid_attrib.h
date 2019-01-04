#ifndef PCB_HID_ATTRIB_H
#define PCB_HID_ATTRIB_H

#include "hid.h"
#include "color.h"

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses int_value, HA_enum sets int_value to the index and
   str_value to the enumeration string.  PCB_HATT_LABEL just shows the
   default str_value. */
struct pcb_hid_attr_val_s {
	int int_value;
	const char *str_value;
	double real_value;
	pcb_coord_t coord_value;
	pcb_color_t clr_value;
	void (*func)();
};

typedef enum pcb_hatt_compflags_e {
	PCB_HATF_FRAME       = 1,  /* box/table has a visible frame around it */
	PCB_HATF_LABEL       = 2,  /* direct children print label */
	PCB_HATF_SCROLL      = 4,  /* box/table is scrollable */
	PCB_HATF_HIDE_TABLAB = 8,  /* hide tab labes of a TABBED - the tab mechanism works, but tab names are not displayed and are not clickable */
	PCB_HATF_CLR_STATIC  = 8,  /* color that can not be changed */
	PCB_HATF_LEFT_TAB    = 16, /* display tab labels of TABBED on the left instead of on top (default) */
	PCB_HATF_TREE_COL    = 32, /* first column of a PCB_HATT_TREE is a tree */
	PCB_HATF_EXPFILL     = 64  /* for hbox and vbox: expand and fill */
} pcb_hatt_compflags_t;

typedef enum pcb_hids_e {
	/* atomic entry types */
	PCB_HATT_LABEL,
	PCB_HATT_INTEGER,
	PCB_HATT_REAL,
	PCB_HATT_STRING,              /* WARNING: string, must be malloc()'d, can't be a (const char *) */
	PCB_HATT_BOOL,
	PCB_HATT_ENUM,
	PCB_HATT_PATH,                /* WARNING: string, must be malloc()'d, can't be a (const char *) */
	PCB_HATT_UNIT,
	PCB_HATT_COORD,
	PCB_HATT_BUTTON,              /* push button; default value is the label */
	PCB_HATT_TREE,                /* tree/list/table view; number of columns: pcb_hatt_table_cols; data is in field 'enumerations' */
	PCB_HATT_PROGRESS,            /* progress bar; displays real_value between 0 and 1 */
	PCB_HATT_PREVIEW,             /* preview/render widget; callbacks in 'enumerations' */
	PCB_HATT_PICTURE,             /* static picture from xpm - picture data in enumerations */
	PCB_HATT_PICBUTTON,           /* button with static picture from xpm - picture data in enumerations */
	PCB_HATT_COLOR,               /* color pick (user input: select a color) */

	/* groups (e.g. boxes) */
	PCB_HATT_BEGIN_HBOX,          /* NOTE: PCB_HATT_IS_COMPOSITE() depends on it */
	PCB_HATT_BEGIN_VBOX,
	PCB_HATT_BEGIN_HPANE,         /* horizontal split and offer two vboxes; the split ratio is real_value between 0 and 1, that describes the left side's size */
	PCB_HATT_BEGIN_VPANE,         /* vertical split and offer two vboxes; the split ratio is real_value between 0 and 1, that describes the left side's size */
	PCB_HATT_BEGIN_TABLE,         /* min_val is the number of columns */
	PCB_HATT_BEGIN_TABBED,        /* tabbed view (e.g. notebook); ->enumerations stores the tab names and a NULL; default_val's integer value is the index of the current tab */
	PCB_HATT_END          /* close one level of PCB_HATT_* */
} pcb_hids_t;

#define PCB_HATT_IS_COMPOSITE(type) \
	(((type) >= PCB_HATT_BEGIN_HBOX) && ((type) < PCB_HATT_END))

#define PCB_HAT_IS_STR(type) \
	((type == PCB_HATT_STRING) || (type == PCB_HATT_PATH))

/* alternative field names in struct pcb_hid_attribute_s */
#define pcb_hatt_flags       max_val
#define pcb_hatt_table_cols  min_val

typedef struct {
	int cols;        /* number of columns used by this node (allocation size) */
	void *hid_data;  /* the hid running the widget can use this field to store a custom pointer per row */
	gdl_list_t children;
	gdl_elem_t link;
	char *path;      /* full path of the node; allocated/free'd by DAD (/ is the root, every entry is specified from the root, but the leading / is omitted; in non-tree case, this only points to the first col data) */
	unsigned hide:1; /* if non-zero, the row is not visible (e.g. filtered out) */

	/* caller/user data */
	void *user_data;
	union {
		void *ptr;
		long lng;
		double dbl;
	} user_data2;
	char *cell[1];   /* each cell is a char *; the true length of the array is the value of the len field; the array is allocated together with the struct */
} pcb_hid_row_t;

typedef struct {
	gdl_list_t rows; /* ordered list of first level rows (tree root) */
	htsp_t paths;    /* translate first column paths iinto (pcb_hid_row_t *) */
	pcb_hid_attribute_t *attrib;
	const char **hdr; /* optional column headers (NULL means disable header) */

	/* optional callbacks the user set after widget creation */
	void *user_ctx;
	void (*user_free_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);
	void (*user_selected_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);
	int (*user_browse_activate_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row); /* returns non-zero if the row should auto-activate while browsing (e.g. stepping with arrow keys) */
	const char *(*user_copy_to_clip_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row); /* returns the string to copy to clipboard for the given row (if unset, first column text is used) */

	/* optional callbacks HIDs may set after widget creation */
	void *hid_ctx;
	void (*hid_insert_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *new_row);
	void (*hid_modify_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row, int col); /* if col is negative, all columns have changed */
	void (*hid_remove_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);
	pcb_hid_row_t *(*hid_get_selected_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx);
	void (*hid_jumpto_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row); /* row = NULL means deselect all */
	void (*hid_expcoll_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row, int expanded); /* sets whether a row is expanded or collapsed */
	void (*hid_update_hide_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx);
} pcb_hid_tree_t;

typedef struct pcb_hid_preview_s pcb_hid_preview_t;
struct pcb_hid_preview_s {
	pcb_hid_attribute_t *attrib;

	pcb_box_t initial_view;
	unsigned initial_view_valid:1;

	int min_sizex_px, min_sizey_px; /* hint: widget minimum size in pixels */

	/* optional callbacks the user set after widget creation */
	void *user_ctx;
	void (*user_free_cb)(pcb_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx);
	void (*user_expose_cb)(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);
	pcb_bool (*user_mouse_cb)(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y); /* returns true if redraw is needed */

	/* optional callbacks HIDs may set after widget creation */
	void *hid_ctx;
	void (*hid_zoomto_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, const pcb_box_t *view);
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx);
};

struct pcb_hid_attribute_s {
	const char *name;
	/* If the help_text is this, usage() won't show this option */
#define ATTR_UNDOCUMENTED ((char *)(1))
	const char *help_text;
	pcb_hids_t type;
	int min_val, max_val;				/* for integer and real */
	pcb_hid_attr_val_t default_val;		/* Also actual value for global attributes.  */

	/* NULL terminated list of values for a PCB_HATT_ENUM;
	   Also (ab)used as (pcb_hid_tree_t *) for a PCB_HATT_TREE and for PCB_HATT_PICTURE & PCB_HATT_PICBUTTON */
	const char **enumerations;

	/* If set, this is used for global attributes (i.e. those set
	   statically with REGISTER_ATTRIBUTES below) instead of changing
	   the default_val. */
	void *value;

	/* dynamic API */
	int changed; /* 0 for initial values, 1 on user change */
	void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr); /* called upon value change by the user */
	void (*enter_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);  /* called upon the user pressed enter in a widget that handles keys */
	void *user_data; /* ignored; the caller is free to use it */
};

extern void pcb_hid_register_attributes(pcb_hid_attribute_t *, int, const char *cookie, int copy);
#define PCB_REGISTER_ATTRIBUTES(a, cookie) PCB_HIDCONCAT(void register_,a) ()\
{ pcb_hid_register_attributes(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* Remove all attributes registered with the given cookie */
void pcb_hid_remove_attributes_by_cookie(const char *cookie);

/* remove all attributes and free the list */
void pcb_hid_attributes_uninit(void);

typedef struct pcb_hid_attr_node_s {
	struct pcb_hid_attr_node_s *next;
	pcb_hid_attribute_t *attributes;
	int n;
	const char *cookie;
} pcb_hid_attr_node_t;

extern pcb_hid_attr_node_t *hid_attr_nodes;

void pcb_hid_usage(pcb_hid_attribute_t * a, int numa);
void pcb_hid_usage_option(const char *name, const char *help);

/* Count the number of direct children, start_from the first children */
int pcb_hid_attrdlg_num_children(pcb_hid_attribute_t *attrs, int start_from, int n_attrs);

/* Invoke a simple modal attribute dialog if GUI is available */
int pcb_attribute_dialog_(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, int *already_freed, int defx, int defy);
int pcb_attribute_dialog(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data);


/* Convert between compflag bit value and name */
const char *pcb_hid_compflag_bit2name(pcb_hatt_compflags_t bit);
pcb_hatt_compflags_t pcb_hid_compflag_name2bit(const char *name);

#endif
