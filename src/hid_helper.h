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

#define SL_0_SIDE	0x0000
#define SL_TOP_SIDE	0x0001
#define SL_BOTTOM_SIDE	0x0002
#define SL_SILK		0x0010
#define SL_MASK		0x0020
#define SL_PDRILL	0x0030
#define SL_UDRILL	0x0040
#define SL_PASTE	0x0050
#define SL_INVISIBLE	0x0060
#define SL_FAB		0x0070
#define SL_ASSY		0x0080
#define SL_RATS		0x0090
/* Callers should use this.  */
#define SL(type,side) (~0xfff | SL_##type | SL_##side##_SIDE)

#endif
