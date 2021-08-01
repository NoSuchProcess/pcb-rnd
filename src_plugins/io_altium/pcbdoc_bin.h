typedef struct {
	long alloced, used;
	unsigned char *data;
} altium_buf_t;

int pcbdoc_bin_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *file_name, FILE *f);
int pcbdoc_bin_parse_file(rnd_hidlib_t *hidlib, altium_tree_t *tree, const char *fn);


