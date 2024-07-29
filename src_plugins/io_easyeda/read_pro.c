/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - dispatch pro format read
 *  pcb-rnd Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust Fund in 2024)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/compat_fs_dir.h>
#include <librnd/core/compat_lrealpath.h>
#include <librnd/core/rotate.h>
#include "../lib_compat_help/pstk_compat.h"
#include "../lib_compat_help/pstk_help.h"
#include "rnd_inclib/lib_svgpath/svgpath.h"

#include "board.h"
#include "data.h"
#include "netlist.h"
#include "obj_subc_parent.h"
#include "plug_io.h"

#include "io_easyeda_conf.h"
#include "easyeda_sphash.h"
#include "read_common.h"

#include <rnd_inclib/lib_easyeda/easyeda_low.c>
#include <rnd_inclib/lib_easyeda/zip_helper.c>
#include <rnd_inclib/lib_geo/arc_sed.c>
#include "read_pro_low.c"
#include "read_pro_hi.c"
#include "read_pro_epro.c"

/* True when the user configured zip commands */
#define CAN_UNZIP ((conf_io_easyeda.plugins.io_easyeda.zip_list_cmd != NULL) && (*conf_io_easyeda.plugins.io_easyeda.zip_list_cmd != '\0'))

/* assume plain text, search for ["DOCTYPE","FOOTPRINT" in the first few lines */
int io_easyeda_pro_test_parse_efoo(pcb_plug_io_t *ctx, pcb_plug_iot_t type, const char *Filename, FILE *f)
{
	int lineno;

	for(lineno = 0; lineno < 8; lineno++) {
		char buff[1024], *line;
		unsigned char *ul;

		if ((line = fgets(buff, sizeof(buff), f)) == NULL)
			return 0; /* refuse */

		/* skip utf-8 bom */
		if (lineno == 0) {
			ul = (unsigned char *)line;
			if ((ul[0] == 0xef) && (ul[1] == 0xbb) && (ul[2] == 0xbf))
				line += 3;
		}

		while(isspace(*line)) line++;
		if (strncmp(line, "[\"DOCTYPE\",\"FOOTPRINT\"", 22) == 0)
			return 1; /* accept */
	}

	return 0;
}

int easyeda_pro_parse_epro_board(pcb_board_t *pcb, const char *Filename, rnd_conf_role_t settings_dest)
{
	char *cmd, *fullpath, *dir;
	const char *prefix[4];
	epro_t epro = {0};
	int res = -1;

	/* unpack: create a temp dir... */
	if (!conf_io_easyeda.plugins.io_easyeda.debug.unzip_static) {
		if (rnd_mktempdir(NULL, &epro.zipdir, "easypro") != 0) {
			rnd_message(RND_MSG_ERROR, "io_easyeda: failed to create temp dir for unpacking zip file '%s'\n", Filename);
			goto error;
		}
		dir = epro.zipdir.array;
	}
	else {
		gds_append_str(&epro.zipdir, "/tmp/easypro");
		dir = epro.zipdir.array;
		rnd_mkdir(&pcb->hidlib, dir, 0755);
	}

	/* ... cd to it and unpacl there with full path to the zip */
	prefix[0] = "cd ";
	prefix[1] = dir;
	prefix[2] = ";";
	prefix[3] = NULL;

	fullpath = rnd_lrealpath(Filename);
	cmd = easypro_zip_cmd(prefix, conf_io_easyeda.plugins.io_easyeda.zip_extract_cmd, fullpath);
	free(fullpath);

	res = rnd_system(NULL, cmd);
	if (res != 0) {
		rnd_message(RND_MSG_ERROR, "io_easyeda: unable to unzip using command: '%s'\nDetails on stderr.\nPlease check your configuration!\n", cmd);
		free(cmd);
		res = -1;
		goto error;
	}
	free(cmd);

	epro.zipname = Filename;
	epro.pcb = pcb;
	epro.settings_dest = settings_dest;

	if (epro_load_project_json(&epro) != 0)
		goto error;

	if (epro_select_board(&epro) != 0)
		goto error;

	res = epro_load_board(&epro);

	error:;
	if (!conf_io_easyeda.plugins.io_easyeda.debug.unzip_static)
		rnd_rmtempdir(NULL, &epro.zipdir);

	epro_uninit(&epro);
	return res;
}

int io_easyeda_pro_test_parse_epro(pcb_plug_iot_t type, const char *Filename, FILE *f)
{
	int res = 0;
	char *cmd;
	FILE *fc;

	if (!easypro_is_file_zip(f))
		return -1;

	/* get a file list and look for known index files */
	cmd = easypro_zip_cmd(NULL, conf_io_easyeda.plugins.io_easyeda.zip_list_cmd, Filename);
	fc = rnd_popen(NULL, cmd, "r");
	if (fc != NULL) {
		char *line, buf[1024];
		while((line = fgets(buf, sizeof(buf), fc)) != NULL) {
			if (strstr(line, "project.json") != NULL) { /* board */
				res = 1;
				break;
			}
		}

		fclose(fc);
	}
	free(cmd);
	return res;
}


int io_easyeda_pro_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t type, const char *Filename, FILE *f)
{
	if (((type == PCB_IOT_FOOTPRINT) || (type == PCB_IOT_PCB)) && (io_easyeda_pro_test_parse_efoo(ctx, type, Filename, f) == 1))
		return 1;
	else
		rewind(f);

	if ((type == PCB_IOT_PCB) && CAN_UNZIP) {
		int res = io_easyeda_pro_test_parse_epro(type, Filename, f);
		if (res == 1)
			return 1;
		else
			rewind(f);
	}
	/* return 1 for accept */
	return 0;
}

int io_easyeda_pro_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, rnd_conf_role_t settings_dest)
{
	FILE *f;
	int is_fp;

	f = rnd_fopen(&pcb->hidlib, Filename, "r");
	if (f == NULL)
		return -1;

	is_fp = (io_easyeda_pro_test_parse_efoo(ctx, PCB_IOT_PCB, Filename, f) == 1);
	if (is_fp) {
		int res;
		rewind(f);
		res = easyeda_pro_parse_fp_as_board(pcb, Filename, f, settings_dest);
		fclose(f);
		return res;
	}
	else
		fclose(f);


	return easyeda_pro_parse_epro_board(pcb, Filename, settings_dest);
}

int io_easyeda_pro_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename, const char *subfpname)
{
	return easyeda_pro_parse_fp(data, filename, 1, NULL);
}

pcb_plug_fp_map_t *io_easyeda_pro_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags)
{
	if (io_easyeda_pro_test_parse(ctx, PCB_IOT_FOOTPRINT, fn, f) != 1)
		return NULL;

	head->type = PCB_FP_FILE;
	head->name = rnd_strdup("first");
	return head;
}
