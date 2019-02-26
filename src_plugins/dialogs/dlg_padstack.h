#include "obj_pstk.h"

typedef struct pse_s pse_t;
struct pse_s {
	/* caller conf */
	int disable_instance_tab, gen_shape_in_place;
	pcb_hid_attribute_t *attrs;
	pcb_board_t *pcb;
	pcb_data_t *data; /* parent data where the proto is sitting; might be a subc */
	pcb_pstk_t *ps;

	/* optional hooks */
	void *user_data;                /* owned by the caller who sets up the struct */
	void (*change_cb)(pse_t *pse);

	/* internal states */
	int tab;

	/* widget IDs */
	int but_instance, but_prototype;
	int proto_id, clearance, rot, xmirror, smirror;
	int proto_shape[pcb_proto_num_layers];
	int proto_info[pcb_proto_num_layers];
	int proto_change[pcb_proto_num_layers];
	pcb_coord_t proto_clr[pcb_proto_num_layers];
	int prname;
	int hole_header;
	int hdia, hplated;
	int htop_val, htop_text, htop_layer;
	int hbot_val, hbot_text, hbot_layer;
	int gen_shp, gen_size, gen_drill, gen_sides, gen_expose, gen_paste, gen_do;

	/* sub-dialog: shape change */
	void *parent_hid_ctx;
	int editing_shape; /* index of the shape being edited */
	pcb_hid_attribute_t *shape_chg;
	int text_shape, del, derive, hshadow;
	int copy_do, copy_from;
	int shrink, amount, grow;
};

void pcb_pstkedit_dialog(pse_t *pse, int target_tab);

extern const char pcb_acts_PadstackEdit[];
extern const char pcb_acth_PadstackEdit[];
fgw_error_t pcb_act_PadstackEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv);
