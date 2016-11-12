#ifndef PCB_HID_HELPER_H
#define PCB_HID_HELPER_H

enum pcb_file_name_style_e {
	/* Files for copper layers are named top, groupN, bottom.  */
	PCB_FNS_fixed,
	/* Groups with multiple layers are named as above, else the single
	   layer name is used.  */
	PCB_FNS_single,
	/* The name of the first layer in each group is used.  */
	PCB_FNS_first
};

/* Returns a filename base that can be used to output the layer.  */
const char *layer_type_to_file_name(int idx, int style);

void derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix, char **memory);

#endif
