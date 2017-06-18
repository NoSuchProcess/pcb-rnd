/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  autotrax IO plugin - plugin coordination
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#include "footprint.h"*/

#include "plugins.h"
#include "plug_io.h"
#include "write.h"
#include "read.h"/*
#include "read_net.h" */
#include "hid_actions.h"

#include "board.h"
#include "netlist.h"
#include "conf_core.h"
#include "buffer.h"
#include "hid.h"
#include "action_helper.h"
#include "compat_misc.h"

static pcb_plug_io_t io_autotrax;
static const char *autotrax_cookie = "autotrax IO";

static const char pcb_acts_Saveautotrax[] = "SaveAutotrax(type, filename)";
static const char pcb_acth_Saveautotrax[] = "Saves the specific type of data in an autotrax file. Type can be: board-footprints";
/*static const char *autotrax_cookie = "Protel Autotrax importer";
*/
static const char pcb_acts_LoadAutotraxFrom[] = "LoadAutotraxFrom(filename)";
static const char pcb_acth_LoadAutotraxFrom[] = "Loads the specified autotrax layout.";


int io_autotrax_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "Protel autotrax") != 0) ||
		((typ & (~(PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER | PCB_IOT_PCB))) != 0))
		return 0;

	return 100;
}

/*PCB_REGISTER_ACTIONS(autotrax_action_list, autotrax_cookie) 
*/


/*
pcb_hid_action_t autotrax_action_list[] = {
        {"LoadAutotraxFrom", 0, pcb_act_LoadAutotraxFrom, pcb_acth_LoadAutotraxFrom, pcb_acts_LoadAutotraxFrom}
};

*/

/*PCB_REGISTER_ACTIONS(autotrax_action_list, autotrax_cookie)

int pplg_check_ver_import_autotrax(int ver_needed) { return 0; }

void pplg_uninit_import_autotrax(void)
{
        pcb_hid_remove_actions_by_cookie(autotrax_cookie);
}
*/
/*#include "dolists.h"
int pplg_init_import_autotrax(void)
{
        PCB_REGISTER_ACTIONS(autotrax_action_list, autotrax_cookie)
        return 0;
}
//////////////////// */



int pplg_check_ver_io_autotrax(int ver_needed) { return 0; }

void pplg_uninit_io_autotrax(void)
{
	pcb_hid_remove_actions_by_cookie(autotrax_cookie);
}

#include "dolists.h"

int pplg_init_io_autotrax(void)
{

	/* register the IO hook */
	io_autotrax.plugin_data = NULL;
	io_autotrax.fmt_support_prio = io_autotrax_fmt;
	io_autotrax.test_parse_pcb = io_autotrax_test_parse_pcb;
	io_autotrax.parse_pcb = io_autotrax_read_pcb;
	io_autotrax.parse_element = NULL;
	io_autotrax.parse_font = NULL;
	io_autotrax.write_buffer = io_autotrax_write_buffer;
	io_autotrax.write_element = io_autotrax_write_element;
	io_autotrax.write_pcb = io_autotrax_write_pcb;
	io_autotrax.default_fmt = "Protel autotrax";
	io_autotrax.description = "Protel autotrax";
	io_autotrax.save_preference_prio = 80;
	io_autotrax.default_extension = ".PCB";
	io_autotrax.fp_extension = ".PCB";
	io_autotrax.mime_type = "application/x-autotrax-pcb";


	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_autotrax);
/*
	PCB_REGISTER_ACTIONS(autotrax_action_list, autotrax_cookie);
*/
	/* TODO: Alloc plugin-globals here. */

	return 0;
}

