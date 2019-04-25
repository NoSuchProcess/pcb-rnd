/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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
 *
 */

#include "config.h"
#include "data.h"
#include "board.h"
#include "conf.h"

#include "menu_helper.h"
#include "genht/hash.h"
#include "genht/htsp.h"
#include "error.h"
#include "actions.h"

int pcb_hid_get_flag(const char *name)
{
	const char *cp;

	if (name == NULL)
		return -1;

	cp = strchr(name, '(');
	if (cp == NULL) {
		conf_native_t *n = conf_get_field(name);
		if (n == NULL)
			return -1;
		if ((n->type != CFN_BOOLEAN) || (n->used != 1))
			return -1;
		return n->val.boolean[0];
	}
	else {
		const char *end, *s;
		fgw_arg_t res, argv[2];
		if (cp != NULL) {
			const pcb_action_t *a;
			fgw_func_t *f;
			char buff[256];
			int len, multiarg;
			len = cp - name;
			if (len > sizeof(buff)-1) {
				pcb_message(PCB_MSG_ERROR, "hid_get_flag: action name too long: %s()\n", name);
				return -1;
			}
			memcpy(buff, name, len);
			buff[len] = '\0';
			a = pcb_find_action(buff, &f);
			if (!a) {
				pcb_message(PCB_MSG_ERROR, "hid_get_flag: no action %s\n", name);
				return -1;
			}
			cp++;
			len = strlen(cp);
			end = NULL;
			multiarg = 0;
			for(s = cp; *s != '\0'; s++) {
				if (*s == ')') {
					end = s;
					break;
				}
				if (*s == ',')
					multiarg = 1;
			}
			if (!multiarg) {
				/* faster but limited way for a single arg */
				if ((len > sizeof(buff)-1) || (end == NULL)) {
					pcb_message(PCB_MSG_ERROR, "hid_get_flag: action arg too long or unterminated: %s\n", name);
					return -1;
				}
				len = end - cp;
				memcpy(buff, cp, len);
				buff[len] = '\0';
				argv[0].type = FGW_FUNC;
				argv[0].val.func = f;
				argv[1].type = FGW_STR;
				argv[1].val.str = buff;
				res.type = FGW_INVALID;
				if (pcb_actionv_(f, &res, (len > 0) ? 2 : 1, argv) != 0)
					return -1;
				fgw_arg_conv(&pcb_fgw, &res, FGW_INT);
				return res.val.nat_int;
			}
			else {
				/* slower but more generic way */
				return pcb_parse_command(name, pcb_true);
			}
		}
		else {
			fprintf(stderr, "ERROR: pcb_hid_get_flag(%s) - not a path or an action\n", name);
		}
	}
	return -1;
}
