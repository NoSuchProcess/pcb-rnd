#ifndef PCB_HID_HELPER_H
#define PCB_HID_HELPER_H

enum File_Name_Style {
	/* Files for copper layers are named top, groupN, bottom.  */
	FNS_fixed,
	/* Groups with multiple layers are named as above, else the single
	   layer name is used.  */
	FNS_single,
	/* The name of the first layer in each group is used.  */
	FNS_first,
};

/* Returns a filename base that can be used to output the layer.  */
const char *layer_type_to_file_name(int idx, int style);

void derive_default_filename(const char *pcbfile, HID_Attribute * filename_attrib, const char *suffix, char **memory);

/* These decode the set_layer index. */
#define SL_TYPE(x) ((x) < 0 ? (x) & 0x0f0 : 0)
#define SL_SIDE(x) ((x) & 0x00f)
#define SL_MYSIDE(x) ((((x) & SL_BOTTOM_SIDE)!=0) == (SWAP_IDENT != 0))

#endif
