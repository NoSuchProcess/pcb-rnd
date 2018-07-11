#ifndef PCB_HID_HELPER_H
#define PCB_HID_HELPER_H

/* maximum size of a derived suffix */
#define PCB_DERIVE_FN_SUFF_LEN 20

typedef enum pcb_file_name_style_e {
	/* Files for copper layers are named top, groupN, bottom.  */
	PCB_FNS_fixed,
	/* Groups with multiple layers are named as above, else the single
	   layer name is used.  */
	PCB_FNS_single,
	/* The name of the first layer in each group is used.  */
	PCB_FNS_first
} pcb_file_name_style_t;

/* Returns a filename base that can be used to output the layer.  */
char *pcb_layer_to_file_name(char *dest, pcb_layer_id_t lid, unsigned int flags, pcb_file_name_style_t style);

/* Returns a filename base that can be used to output the layer; if flags is 0,
   look it up. Copies result in dest (which should be at least PCB_DERIVE_FN_SUFF_LEN bytes wide).
   Returns dest. */
void pcb_derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix, char **memory);

/*** CAM API ***/
typedef struct pcb_cam_s {
	/* public */
	const char *fn;
	pcb_bool active;
	int grp_vis[PCB_MAX_LAYERGRP]; /* whether a layer group should be rendered */

	/* private/internal/cache */
	char *inst;
	pcb_board_t *pcb;
	int orig_vis[PCB_MAX_LAYER]; /* backup of the original layer visibility */
} pcb_cam_t;

int pcb_cam_begin(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const pcb_hid_attribute_t *attr_tbl, int numa, pcb_hid_attr_val_t *options);
void pcb_cam_end(pcb_cam_t *dst);

/* Shall be the first rule in a cam capable exporter's set_layer_group()
   callback: decides not to draw a layer group in cam mode if the
   group is not scheduled for export */
#define pcb_cam_set_layer_group(cam, group, flags) \
do { \
	pcb_cam_t *__cam__ = (cam); \
	if (!__cam__->active) \
		break; \
	if ((group) == -1) \
		return 0; \
	if (!__cam__->grp_vis[(group)]) \
		return 0; \
} while(0)

#endif
