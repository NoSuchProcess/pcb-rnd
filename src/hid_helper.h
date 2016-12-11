#ifndef PCB_HID_HELPER_H
#define PCB_HID_HELPER_H

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
   look it up. Copies result in dest (which should be at least TODO bytes wide).
   Returns dest. */
void pcb_derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix, char **memory);

#endif
