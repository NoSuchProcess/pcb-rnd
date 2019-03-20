#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "fuse_includes.h"

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"

#include "hid.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid_nogui.h"

#include "../src_plugins/lib_vfs/lib_vfs.h"

const char *export_vfs_fuse_cookie = "export_vfs_fuse HID";

static pcb_hid_attribute_t *export_vfs_fuse_get_export_options(int *n)
{
	return 0;
}


static int pcb_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	return -1;
}

static int pcb_fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	return -1;
}

static int pcb_fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	return -1;
}

static int pcb_fuse_open(const char *path, struct fuse_file_info *fi)
{
	return -1;
}

static char **fuse_argv;
static int fuse_argc = 0;
static void export_vfs_fuse_do_export(pcb_hid_attr_val_t *options)
{
	static struct fuse_operations oper;

	oper.readdir = pcb_fuse_readdir;
	oper.open = pcb_fuse_open;
	oper.read = pcb_fuse_read;
	oper.write = pcb_fuse_write;

	if (fuse_main(fuse_argc, fuse_argv, &oper, NULL) != 0)
		fprintf(stderr, "fuse_main() returned error.\n");
}

static int export_vfs_fuse_usage(const char *topic)
{
	fprintf(stderr, "\nexport_vfs_fuse exporter command line arguments are plain fuse aguments.\n\n");
	fprintf(stderr, "\nUsage: pcb-rnd [pcb-rnd-options] [-o fuse-options] foo.pcb mountpoint\n\n");
	return 0;
}

#define arg_append(dst_argc, dst_argv, str)   dst_argv[dst_argc++] = str

#define arg_copy_to_fuse(cnt) \
do { \
	int __i__; \
	for(__i__ = 0; __i__ < (cnt); __i__++,n++) \
		arg_append(fuse_argc, fuse_argv, in_argv[n]); \
	n--; \
} while(0)

#define arg_copy_to_pcb(cnt) \
do { \
	int __i__; \
	for(__i__ = 0; __i__ < (cnt); __i__++,n++) \
		arg_append(fuse_ret_argc, fuse_ret_argv, in_argv[n]); \
	n--; \
} while(0)

static char **fuse_ret_argv;
static int fuse_ret_argc = 0;
static int export_vfs_fuse_parse_arguments(int *argc, char ***argv)
{
	int n, in_argc = *argc;
	char **in_argv = *argv;
	char *board = NULL, *dir = NULL;

	fuse_argv = malloc(sizeof(char *) * (*argc+1));
	fuse_argc = 0;
	fuse_ret_argv = malloc(sizeof(char *) * (*argc+1));
	fuse_ret_argc = 0;

	for(n = 0; n < in_argc; n++) {
		if (strcmp(in_argv[n], "-o") == 0)   arg_copy_to_fuse(2);
		else if (*in_argv[n] == '-')         arg_copy_to_pcb(2);
		else {
			if (board == NULL)     board = in_argv[n];
			else if (dir == NULL)  dir = in_argv[n];
			else {
				fprintf(stderr, "export_vfs_fuse: excess unrecognized argument: '%s'\n", in_argv[n]);
				exit(1);
			}
		}
	}

	arg_append(fuse_ret_argc, fuse_ret_argv, board);
	arg_append(fuse_argc, fuse_argv, dir);

	fuse_argv[fuse_argc] = NULL;
	fuse_ret_argv[fuse_ret_argc] = NULL;

	argc = &fuse_ret_argc;
	*argv = fuse_ret_argv;
	return 0;
}

pcb_hid_t export_vfs_fuse_hid;

int pplg_check_ver_export_vfs_fuse(int ver_needed) { return 0; }

void pplg_uninit_export_vfs_fuse(void)
{
	pcb_hid_remove_attributes_by_cookie(export_vfs_fuse_cookie);
	free(fuse_argv);
	fuse_argv = NULL;
	free(fuse_ret_argv);
	fuse_ret_argv = NULL;
}

int pplg_init_export_vfs_fuse(void)
{
	PCB_API_CHK_VER;

	memset(&export_vfs_fuse_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&export_vfs_fuse_hid);

	export_vfs_fuse_hid.struct_size = sizeof(pcb_hid_t);
	export_vfs_fuse_hid.name = "vfs_fuse";
	export_vfs_fuse_hid.description = "Handler of FUSE calls, serving board data";
	export_vfs_fuse_hid.exporter = 1;
	export_vfs_fuse_hid.hide_from_gui = 1;

	export_vfs_fuse_hid.get_export_options = export_vfs_fuse_get_export_options;
	export_vfs_fuse_hid.do_export = export_vfs_fuse_do_export;
	export_vfs_fuse_hid.parse_arguments = export_vfs_fuse_parse_arguments;

	export_vfs_fuse_hid.usage = export_vfs_fuse_usage;

	pcb_hid_register_hid(&export_vfs_fuse_hid);
	return 0;
}
