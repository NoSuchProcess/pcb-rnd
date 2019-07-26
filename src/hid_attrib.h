#ifndef PCB_HID_ATTRIB_H
#define PCB_HID_ATTRIB_H

#include <genlist/gendlist.h>
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
	PCB_HATF_TIGHT       = 2,  /* box/table/button has minimal padding and spacing */
	PCB_HATF_SCROLL      = 4,  /* box/table is scrollable */
	PCB_HATF_HIDE_TABLAB = 8,  /* hide tab labes of a TABBED - the tab mechanism works, but tab names are not displayed and are not clickable */
	PCB_HATF_CLR_STATIC  = 8,  /* color that can not be changed */
	PCB_HATF_LEFT_TAB    = 16, /* display tab labels of TABBED on the left instead of on top (default) */
	PCB_HATF_TREE_COL    = 32, /* first column of a PCB_HATT_TREE is a tree */
	PCB_HATF_EXPFILL     = 64, /* for hbox and vbox: expand and fill */
	PCB_HATF_HIDE        = 128,/* widget is initially hidden */
	PCB_HATF_TOGGLE      = 256,/* for buttons and picbuttons: use a toggle button instead of a push button */
	PCB_HATF_TEXT_TRUNCATED = 512, /* label: do not set widget size for text, truncate text if widget is smaller */
	PCB_HATF_TEXT_VERTICAL  = 1024,/* label: rotate text 90 degrees so it can be read from the right */
	PCB_HATF_PRV_BOARD   = 2048/* indicates that a preview widget is showing a section of the board so it needs to be redrawn when the board is redrawn */
} pcb_hatt_compflags_t;

typedef enum pcb_hids_e {
	/* atomic entry types */
	PCB_HATT_LABEL,
	PCB_HATT_INTEGER,
	PCB_HATT_REAL,
	PCB_HATT_STRING,              /* WARNING: string, must be malloc()'d, can't be a (const char *) */
	PCB_HATT_BOOL,
	PCB_HATT_ENUM,
	PCB_HATT_UNIT,
	PCB_HATT_COORD,
	PCB_HATT_BUTTON,              /* push button; default value is the label */
	PCB_HATT_TREE,                /* tree/list/table view; number of columns: pcb_hatt_table_cols; data is in field 'enumerations' */
	PCB_HATT_PROGRESS,            /* progress bar; displays real_value between 0 and 1 */
	PCB_HATT_PREVIEW,             /* preview/render widget; callbacks in 'enumerations' */
	PCB_HATT_PICTURE,             /* static picture from xpm - picture data in enumerations */
	PCB_HATT_PICBUTTON,           /* button with static picture from xpm - picture data in enumerations */
	PCB_HATT_COLOR,               /* color pick (user input: select a color) */
	PCB_HATT_TEXT,                /* plain text editor; data is in 'enumerations' as pcb_hid_text_t */

	/* groups (e.g. boxes) */
	PCB_HATT_BEGIN_HBOX,          /* NOTE: PCB_HATT_IS_COMPOSITE() depends on it */
	PCB_HATT_BEGIN_VBOX,
	PCB_HATT_BEGIN_HPANE,         /* horizontal split and offer two vboxes; the split ratio is real_value between 0 and 1, that describes the left side's size */
	PCB_HATT_BEGIN_VPANE,         /* vertical split and offer two vboxes; the split ratio is real_value between 0 and 1, that describes the left side's size */
	PCB_HATT_BEGIN_TABLE,         /* min_val is the number of columns */
	PCB_HATT_BEGIN_TABBED,        /* tabbed view (e.g. notebook); ->enumerations stores the tab names and a NULL; default_val's integer value is the index of the current tab */
	PCB_HATT_BEGIN_COMPOUND,      /* subtree emulating a single widget; (pcb_hid_compound_t *) stored in END's enumerations */
	PCB_HATT_END          /* close one level of PCB_HATT_* */
} pcb_hids_t;

#define PCB_HATT_IS_COMPOSITE(type) \
	(((type) >= PCB_HATT_BEGIN_HBOX) && ((type) < PCB_HATT_END))

#define PCB_HAT_IS_STR(type) (type == PCB_HATT_STRING)

/* alternative field names in struct pcb_hid_attribute_s */
#define pcb_hatt_flags       hatt_flags
#define pcb_hatt_table_cols  min_val

typedef enum {
	PCB_HID_TEXT_INSERT,           /* insert at cursor or replace selection */
	PCB_HID_TEXT_REPLACE,          /* replace the entire text */
	PCB_HID_TEXT_APPEND,           /* append to the end of the text */

	/* modifiers (bitfield) */
	PCB_HID_TEXT_MARKUP = 0x0010   /* interpret minimal html-like markup - some HIDs may ignore these */
} pcb_hid_text_set_t;

typedef struct {
	/* cursor manipulation callbacks */
	void (*hid_get_xy)(pcb_hid_attribute_t *attrib, void *hid_ctx, long *x, long *y); /* can be very slow */
	long (*hid_get_offs)(pcb_hid_attribute_t *attrib, void *hid_ctx);
	void (*hid_set_xy)(pcb_hid_attribute_t *attrib, void *hid_ctx, long x, long y); /* can be very slow */
	void (*hid_set_offs)(pcb_hid_attribute_t *attrib, void *hid_ctx, long offs);
	void (*hid_scroll_to_bottom)(pcb_hid_attribute_t *attrib, void *hid_ctx);
	void (*hid_set_text)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_text_set_t how, const char *txt);
	char *(*hid_get_text)(pcb_hid_attribute_t *attrib, void *hid_ctx); /* caller needs to free the result */
	void (*hid_set_readonly)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_bool readonly); /* by default text views are not read-only */

	/* optional callbacks the user set after widget creation */
	void *user_ctx;
	void (*user_free_cb)(pcb_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx);

	/* optional callbacks HIDs may set after widget creation */
	void *hid_wdata;
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
} pcb_hid_text_t;


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
	void *hid_wdata;
	void (*hid_insert_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *new_row);
	void (*hid_modify_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int col); /* if col is negative, all columns have changed */
	void (*hid_remove_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row);
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row);
	pcb_hid_row_t *(*hid_get_selected_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
	void (*hid_jumpto_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row); /* row = NULL means deselect all */
	void (*hid_expcoll_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int expanded); /* sets whether a row is expanded or collapsed */
	void (*hid_update_hide_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
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
	void *hid_wdata;
	void (*hid_zoomto_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, const pcb_box_t *view);
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
};

typedef struct {
	int wbegin, wend; /* widget index to the correspoding PCB_HATT_BEGIN_COMPOUND and PCB_HATT_END */

	/* compound implementation callbacks */
	int (*widget_state)(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled);
	int (*widget_hide)(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide);
	int (*set_value)(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val); /* set value runtime */
	void (*set_val_num)(pcb_hid_attribute_t *attr, long l, double d, pcb_coord_t c); /* set value during creation; attr is the END */
	void (*set_val_ptr)(pcb_hid_attribute_t *attr, void *ptr); /* set value during creation; attr is the END */
	void (*set_help)(pcb_hid_attribute_t *attr, const char *text); /* set the tooltip help; attr is the END */
	void (*set_field_num)(pcb_hid_attribute_t *attr, const char *fieldname, long l, double d, pcb_coord_t c); /* set value during creation; attr is the END */
	void (*set_field_ptr)(pcb_hid_attribute_t *attr, const char *fieldname, void *ptr); /* set value during creation; attr is the END */
	void (*free)(pcb_hid_attribute_t *attrib); /* called by DAD on free'ing the PCB_HATT_BEGIN_COMPOUND and PCB_HATT_END_COMPOUND widget */
} pcb_hid_compound_t;



struct pcb_hid_attribute_s {
	const char *name;
	const char *help_text;
	pcb_hids_t type;
	int min_val, max_val;				/* for integer and real */
	pcb_hid_attr_val_t default_val;		/* Also actual value for global attributes.  */

	/* NULL terminated list of values for a PCB_HATT_ENUM;
	   Also (ab)used as (pcb_hid_tree_t *) for a PCB_HATT_TREE and for PCB_HATT_PICTURE & PCB_HATT_PICBUTTON */
	const char **enumerations;

	/* dynamic API */
	unsigned changed:1; /* 0 for initial values, 1 on user change */
	unsigned empty:1;   /* set to 1 by the widget implementation if the textual value is empty, where applicable */
	void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr); /* called upon value change by the user */
	void (*right_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr); /* called upon right click by the user */
	void (*enter_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);  /* called upon the user pressed enter in a widget that handles keys */
	void *user_data; /* ignored; the caller is free to use it */
	unsigned int hatt_flags;
};

struct pcb_export_opt_s {
	const char *name;
	const char *help_text;
	pcb_hids_t type;
	double min_val, max_val;        /* for integer and real */
	pcb_hid_attr_val_t default_val;
	const char **enumerations; /* NULL terminated list of values for a PCB_HATT_ENUM */


	/* If set, this is used for global attributes (i.e. those set
	   statically with REGISTER_ATTRIBUTES below) instead of changing
	   the default_val. */
	void *value;
};

extern void pcb_hid_register_attributes(pcb_export_opt_t *, int, const char *cookie, int copy);
#define PCB_REGISTER_ATTRIBUTES(a, cookie) PCB_HIDCONCAT(void register_,a) ()\
{ pcb_hid_register_attributes(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* Remove all attributes registered with the given cookie */
void pcb_hid_remove_attributes_by_cookie(const char *cookie);

/* remove all attributes and free the list */
void pcb_hid_attributes_uninit(void);

typedef struct pcb_hid_attr_node_s {
	struct pcb_hid_attr_node_s *next;
	pcb_export_opt_t *opts;
	int n;
	const char *cookie;
} pcb_hid_attr_node_t;

extern pcb_hid_attr_node_t *hid_attr_nodes;

void pcb_hid_usage(pcb_export_opt_t *a, int numa);
void pcb_hid_usage_option(const char *name, const char *help);

/* Count the number of direct children, start_from the first children */
int pcb_hid_attrdlg_num_children(pcb_hid_attribute_t *attrs, int start_from, int n_attrs);

/* Invoke a simple modal attribute dialog if GUI is available */
int pcb_attribute_dialog_(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, void **retovr, int defx, int defy, int minx, int miny, void **hid_ctx_out);
int pcb_attribute_dialog(const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data);


/* Convert between compflag bit value and name */
const char *pcb_hid_compflag_bit2name(pcb_hatt_compflags_t bit);
pcb_hatt_compflags_t pcb_hid_compflag_name2bit(const char *name);

#endif
