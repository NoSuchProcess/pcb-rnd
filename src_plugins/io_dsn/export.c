/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format write
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
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
#include <librnd/core/plugins.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_nogui.h>
#include <librnd/hid/hid_attrib.h>
#include <librnd/core/safe_fs.h>

#include "board.h"
#include "conf_core.h"
#include "hid_cam.h"
#include "plug_io.h"

#include "write.h"


static const char *dsn_cookie = "dsn importer/ses";

static rnd_hid_t dsn_hid;

static const rnd_export_opt_t dsn_options[] = {
	{"dsnfile", "SPECCTRA DSN file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_dsnfile 0
	{"trackwidth", "track width in mils (0=use pen's)",
	 RND_HATT_COORD, RND_MIL_TO_COORD(0), RND_MIL_TO_COORD(100), {0, 0, 0, 0}, 0},
#define HA_trackwidth 1
	{"clearance", "clearance in mils (0=use pen's)",
	 RND_HATT_COORD, RND_MIL_TO_COORD(0), RND_MIL_TO_COORD(100), {0, 0, 0, 0}, 0},
#define HA_clearance 2
	{"viaproto", "via prototype override (-1=use pen's)",
	 RND_HATT_INTEGER, RND_MIL_TO_COORD(0), 100000, {-1, 0, 0, -1}, 0},
#define HA_viaproto 3
};

#define NUM_OPTIONS (sizeof(dsn_options)/sizeof(dsn_options[0]))

static rnd_hid_attr_val_t dsn_values[NUM_OPTIONS];

static const char *dsn_filename;

static const rnd_export_opt_t *dsn_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *val = dsn_values[HA_dsnfile].str;
	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &dsn_values[HA_dsnfile], ".dsn");
	if (n)
		*n = NUM_OPTIONS;
	return dsn_options;
}

#define rnd_conf_force_set_coord(var, val) *((RND_CFT_COORD *)(&var)) = val
#define rnd_conf_force_set_int(var, val) *((RND_CFT_INTEGER *)(&var)) = val


static void dsn_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	FILE *f;
	int restore_conf = 0;

	if (!options) {
		dsn_get_export_options(hid, 0, design, appspec);
		options = dsn_values;
	}
	dsn_filename = options[HA_dsnfile].str;
	if (!dsn_filename)
		dsn_filename = "pcb-rnd-out.dsn";

	f = rnd_fopen(&PCB->hidlib, dsn_filename, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "dsn export: can not open '%s' for write\n", dsn_filename);
		return;
	}

	if (options[HA_trackwidth].crd > 0) {
		rnd_conf_force_set_coord(conf_core.design.line_thickness, options[HA_trackwidth].crd);
		restore_conf = 1;
	}
	if (options[HA_clearance].crd > 0) {
		rnd_conf_force_set_coord(conf_core.design.clearance, options[HA_clearance].crd);
		restore_conf = 1;
	}
	if (options[HA_viaproto].lng >= 0) {
		rnd_conf_force_set_coord(conf_core.design.via_proto, options[HA_viaproto].lng);
		restore_conf = 1;
	}

	io_dsn_write_pcb(NULL, f, NULL, dsn_filename, 0);

	if (restore_conf)
		rnd_conf_update(NULL, -1);

	fclose(f);
}

static int dsn_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	return rnd_hid_parse_command_line(argc, argv);
}


void pcb_dsn_export_uninit(void)
{
	rnd_export_remove_opts_by_cookie(dsn_cookie);
	rnd_hid_remove_hid(&dsn_hid);
}

void pcb_dsn_export_init(void)
{
	memset(&dsn_hid, 0, sizeof(rnd_hid_t));
	rnd_hid_nogui_init(&dsn_hid);

	dsn_hid.struct_size = sizeof(rnd_hid_t);
	dsn_hid.name = "dsn";
	dsn_hid.description = "Exports DSN format";
	dsn_hid.exporter = 1;
	dsn_hid.get_export_options = dsn_get_export_options;
	dsn_hid.do_export = dsn_do_export;
	dsn_hid.parse_arguments = dsn_parse_arguments;
	dsn_hid.argument_array = dsn_values;

	rnd_hid_register_hid(&dsn_hid);
	rnd_hid_load_defaults(&dsn_hid, dsn_options, NUM_OPTIONS);

	rnd_export_register_opts2(&dsn_hid, dsn_options, sizeof(dsn_options) / sizeof(dsn_options[0]), dsn_cookie, 0);
}

