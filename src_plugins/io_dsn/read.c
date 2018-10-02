/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format read, parser
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "plug_io.h"
#include "error.h"
#include "pcb_bool.h"

#include "read.h"

int io_dsn_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;
	int phc = 0, in_pcb = 0, lineno = 0;

	if (typ != PCB_IOT_PCB)
		return 0;

	while(!(feof(f)) && (lineno < 512)) {
		if (fgets(line, sizeof(line), f) != NULL) {
			lineno++;
			for(s = line; *s != '\0'; s++)
				if (*s == '(')
					phc++;
			s = line;
			if ((phc > 0) && (strstr(s, "pcb") != 0))
				in_pcb = 1;
			if ((phc > 2) && in_pcb && (strstr(s, "space_in_quoted_tokens") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_cad") != 0))
				return 1;
			if ((phc > 2) && in_pcb && (strstr(s, "host_version") != 0))
				return 1;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;

}

int io_dsn_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	pcb_message(PCB_MSG_ERROR, "io_dsn_parse_pcb() not yet implemented.\n");
	return -1;
}

