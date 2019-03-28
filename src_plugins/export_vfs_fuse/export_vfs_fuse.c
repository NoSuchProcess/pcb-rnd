#define _XOPEN_SOURCE 500

#include "fuse_includes.h"

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid_nogui.h"

#include "../src_plugins/lib_vfs/lib_vfs.h"

static const char *export_vfs_fuse_cookie = "export_vfs_fuse HID";
static FILE *flog;
static int pcb_fuse_changed;

static pcb_hid_attribute_t *export_vfs_fuse_get_export_options(int *n)
{
	return 0;
}

typedef struct {
	const char *path;
	void *buf;
	fuse_fill_dir_t filler;
	int pathlen;
} pcb_fuse_list_t;

static void pcb_fuse_list_cb(void *ctx_, const char *path, int isdir)
{
	pcb_fuse_list_t *ctx = ctx_;
	int child, direct, pl;
	const char *attrs = isdir ? "drwxr-xr-x" : "-rw-r--r--";

	pl = strlen(path);
	if (pl <= ctx->pathlen)
		return;

	if (ctx->pathlen > 0) {
		child = memcmp(path, ctx->path, ctx->pathlen) == 0;
		path += ctx->pathlen;
		if (*path == '/')
			path++;
		else if (*path != '\0')
			child = 0;
	}
	else
		child = 1;

	if ((child) && (*path != '\0')) {
		direct = (strchr(path, '/') == NULL);
		if (direct) {
			struct stat st;
			memset(&st, 0, sizeof(struct stat));
			st.st_mode = (isdir ? S_IFDIR : S_IFREG) | 0755;
			st.st_nlink = 1 + !!isdir;
			ctx->filler(ctx->buf, path, &st, 0);
			fprintf(flog, "list_cb ctx->path=%s path=%s\n", ctx->path, path);
			fflush(flog);
		}
	}
}

static int pcb_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	pcb_fuse_list_t ctx;

	if (*path == '/')
		path++;

	ctx.path = path;
	ctx.buf = buf;
	ctx.filler = filler;
	ctx.pathlen = strlen(path);

	fprintf(flog, "LIST path=%s {\n", path);
	fflush(flog);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	pcb_vfs_list(PCB, pcb_fuse_list_cb, &ctx);

	fprintf(flog, "}\n");
	fflush(flog);
	return 0;
}

static int pcb_fuse_getattr(const char *path, struct stat *stbuf)
{
	int isdir;
	gds_t data;

	fprintf(flog, "getattr path=%s\n", path);
	fflush(flog);

	gds_init(&data);
	if (strcmp(path, "/") != 0) {
		if (*path == '/')
			path++;
		if (pcb_vfs_access(PCB, path, &data, 0, &isdir) != 0) {
			fprintf(flog, "   ->   path=%s ENOENT\n", path);
			fflush(flog);
			gds_uninit(&data);
			return -ENOENT;
		}
	}
	else
		isdir = 1;

	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_mode = (isdir ? S_IFDIR : S_IFREG) | 0755;
	stbuf->st_nlink = 1 + !!isdir;
	stbuf->st_size = data.used;

	gds_uninit(&data);
	return 0;
}

static int pcb_fuse_access(const char *path, int mask)
{
	fprintf(flog, "access path=%s %x\n", path, mask);
	fflush(flog);
	return 0;
}

static int pcb_fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	gds_t data;
	int len;

	fprintf(flog, "read path=%s\n", path);
	fflush(flog);

	if (*path == '/')
		path++;
	gds_init(&data);
	if (pcb_vfs_access(PCB, path, &data, 0, NULL) != 0) {
		gds_uninit(&data);
		fprintf(flog, "   ->   path=%s ENOENT\n", path);
		fflush(flog);
		return -ENOENT;
	}

	len = data.used;
	len -= offset;
	if (len > 0)
		memcpy(buf, data.array + offset, len);
	else
		len = 0;
	gds_uninit(&data);

	return len;
}

static int pcb_fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	gds_t data, src;

	fprintf(flog, "write path=%s\n", path);
	fflush(flog);

	if (*path == '/')
		path++;

	gds_init(&data);

	if (offset > 0) {
		if (pcb_vfs_access(PCB, path, &data, 0, NULL) != 0) {
			gds_uninit(&data);
			fprintf(flog, "   ->   path=%s ENOENT\n", path);
			fflush(flog);
			return -ENOENT;
		}
		src.array = (char *)buf;
		src.used = src.alloced = size;
		src.no_realloc = 1;
		gds_copy(&data, offset, &src, 0, size);
	}
	else
		gds_append_len(&data, buf, size);

	if ((data.used > 0) && (data.array[data.used-1] == '\n'))
		data.array[data.used-1] = '\0';

	if (pcb_vfs_access(PCB, path, &data, 1, NULL) != 0) {
		gds_uninit(&data);
		fprintf(flog, "   ->   path=%s EIO\n", path);
		fflush(flog);
		return -EIO;
	}

	gds_uninit(&data);
	pcb_fuse_changed = 1;
	return size;
}

static int pcb_fuse_truncate(const char *path, off_t size)
{
	gds_t data;

	if (*path == '/')
		path++;
	gds_init(&data);
	if (pcb_vfs_access(PCB, path, &data, 0, NULL) != 0) {
		gds_uninit(&data);
		fprintf(flog, "   ->   path=%s ENOENT\n", path);
		fflush(flog);
		return -ENOENT;
	}

	if ((size == data.used) || (size < 0))
		return 0;

	if (size < data.used) {
		data.used = size;
		data.array[size] = '\0';
	}
	else {
		size_t old = data.used;
		gds_enlarge(&data, size);
		if ((old > 0)&&  (data.array[old-1] == '\n'))
			data.array[old-1] = ' ';
		memset(&data.array[old-1], ' ', size-old-1);
		data.array[size] = '\0';
	}

	if (pcb_vfs_access(PCB, path, &data, 1, NULL) != 0) {
		gds_uninit(&data);
		fprintf(flog, "   ->   path=%s EIO\n", path);
		fflush(flog);
		return -EIO;
	}

	gds_uninit(&data);
	pcb_fuse_changed = 1;
	return 0;
}


static int pcb_fuse_open(const char *path, struct fuse_file_info *fi)
{
	fprintf(flog, "open path=%s\n", path);
	fflush(flog);
	return 0;
}

static void pcb_fuse_destroy(void *private_data)
{
	if (!pcb_fuse_changed)
		return;
	if (pcb_save_pcb(PCB->Filename, NULL) != 0) {
		fprintf(stderr, "Failed to save the modified board file\n");
		exit(1);
	}
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
	oper.truncate = pcb_fuse_truncate;
	oper.getattr = pcb_fuse_getattr;
	oper.access = pcb_fuse_access;
	oper.destroy = pcb_fuse_destroy;

	pcb_fuse_changed = 0;
	flog = pcb_fopen("LOG.fuse", "w");
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

	arg_append(fuse_argc, fuse_argv, "pcb-rnd"); /* program name */
	arg_append(fuse_argc, fuse_argv, "?");       /* make room for the mount point */

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

	if (board == NULL) {
		fprintf(stderr, "export_vfs_fuse: board name is required\n");
		exit(1);
	}

	if (dir == NULL) {
		fprintf(stderr, "export_vfs_fuse: mount point is required\n");
		exit(1);
	}

	arg_append(fuse_ret_argc, fuse_ret_argv, board);
	fuse_argv[1] = dir;

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
