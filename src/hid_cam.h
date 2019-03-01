#ifndef PCB_HID_HELPER_H
#define PCB_HID_HELPER_H

#include "layer.h"
#include "hid_attrib.h"

/*** CAM API ***/
typedef struct pcb_cam_s {
	/* public */
	char *fn;
	pcb_bool fn_changed;           /* true if last call changed the ->fn field */
	pcb_bool active;
	int grp_vis[PCB_MAX_LAYERGRP]; /* whether a real layer group should be rendered */
	int vgrp_vis[PCB_VLY_end];     /* whether a virtual layer group should be rendered */

	pcb_xform_t *xform[PCB_MAX_LAYERGRP];
	pcb_xform_t xform_[PCB_MAX_LAYERGRP];
	pcb_xform_t *vxform[PCB_VLY_end];
	pcb_xform_t vxform_[PCB_VLY_end];

	/* private/internal/cache */
	const char *fn_template;
	char *fn_last;
	char *inst;
	pcb_board_t *pcb;
	int orig_vis[PCB_MAX_LAYER]; /* backup of the original layer visibility */
	int exported_grps;
} pcb_cam_t;

int pcb_cam_begin(pcb_board_t *pcb, pcb_cam_t *dst, const char *src, const pcb_hid_attribute_t *attr_tbl, int numa, pcb_hid_attr_val_t *options);

/* Finish cam export, free all memory, mark cam export inactive and report
   the number of layer groups exported */
int pcb_cam_end(pcb_cam_t *dst);


/* Shall be the first rule in a cam capable exporter's set_layer_group()
   callback: decides not to draw a layer group in cam mode if the
   group is not scheduled for export */
#define pcb_cam_set_layer_group(cam, group, purpose, purpi, flags, xform) \
do { \
	if (pcb_cam_set_layer_group_(cam, group, purpose, purpi, flags, xform)) \
		return 0; \
} while(0)

/* the logics behind pcb_cam_set_layer_group(); returns non-zero if the macro
   should return (and skip the current group) */
int pcb_cam_set_layer_group_(pcb_cam_t *cam, pcb_layergrp_id_t group, const char *purpose, int purpi, unsigned int flags, pcb_xform_t **xform);

/*** Obsolete file suffix API - new plugins should not use this ***/
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
char *pcb_layer_to_file_name(char *dest, pcb_layer_id_t lid, unsigned int flags, const char *purpose, int purpi, pcb_file_name_style_t style);

/* Returns a filename base that can be used to output the layer; if flags is 0,
   look it up. Copies result in dest (which should be at least PCB_DERIVE_FN_SUFF_LEN bytes wide). */
void pcb_derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix);

#endif
