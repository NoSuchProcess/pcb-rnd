/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  hand assembly assistant GUI
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

#include <stdio.h>
#include <genvector/gds_char.h>
#include <genlist/gendlist.h>

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "actions.h"
#include "compat_misc.h"
#include "obj_subc.h"
#include "pcb-printf.h"

static const char *asm_cookie = "asm plugin";
static char *sort_template  = "a.footprint, a.value, a.asm_group, side, x, y";
static char *group_template = "a.footprint, a.value, a.asm_group";

typedef enum {
	TT_ATTR,
	TT_SIDE,
	TT_X,
	TT_Y
} ttype_t;

typedef struct {
	ttype_t type;
	const char *key;
	gdl_elem_t link;
} template_t;

static void templ_append(gdl_list_t *dst, ttype_t type, const char *key)
{
	template_t *t = calloc(sizeof(template_t), 1);
	t->type = type;
	t->key = key;
	gdl_append(dst, t, link);
}

static char *templ_compile(gdl_list_t *dst, const char *src_)
{
	char *s, *next, *src = pcb_strdup(src_);

	for(s = src; (s != NULL) && (*s != '\0'); s = next) {
		while(isspace(*s) || (*s == ',')) s++;
		next = strpbrk(s, " \t\r\n,");
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		if ((s[0] == 'a') && (s[1] == '.')) {
			s+=2;
			templ_append(dst, TT_ATTR, s);
		}
		else if (strcmp(s, "side") == 0)
			templ_append(dst, TT_SIDE, NULL);
		else if (strcmp(s, "x") == 0)
			templ_append(dst, TT_X, NULL);
		else if (strcmp(s, "y") == 0)
			templ_append(dst, TT_Y, NULL);
		else
			pcb_message(PCB_MSG_ERROR, "Ignoring unknown asm template entry: '%s'\n", s);
	}
	return src;
}

static char *templ_exec(pcb_subc_t *subc, gdl_list_t *temp)
{
	gds_t s;
	template_t *t;
	int n, bot, have_coords = 0;
	char *tmp, buf[64];
	pcb_coord_t x = 0, y = 0;

	gds_init(&s);
	for(n = 0, t = gdl_first(temp); t != NULL; n++, t = gdl_next(temp, t)) {
		if (n != 0)
			gds_append(&s, ',');
		switch(t->type) {
			case TT_ATTR:
				tmp = pcb_attribute_get(&subc->Attributes, t->key);
				if (tmp != NULL)
					gds_append_str(&s, tmp);
				break;
			case TT_SIDE:
				bot = 0;
				pcb_subc_get_side(subc, &bot);
				gds_append_str(&s, (bot ? "1/bottom" : "0/top"));
				break;
			case TT_X:
				if (!have_coords) {
					pcb_subc_get_origin(subc, &x, &y);
					have_coords = 1;
				}
				pcb_sprintf(buf, "%.08mm", x);
				break;
			case TT_Y:
				if (!have_coords) {
					pcb_subc_get_origin(subc, &x, &y);
					have_coords = 1;
				}
				pcb_sprintf(buf, "%.08mm", y);
				break;
		}
	}
	return s.array;
}

static void templ_free(char *tmp, gdl_list_t *dst)
{
	template_t *t;

	for(t = gdl_first(dst); t != NULL; t = gdl_first(dst)) {
		gdl_remove(dst, t, link);
		free(t);
	}

	free(tmp);
}

static void asm_extract(pcb_data_t *data, const char *group_template, const char *sort_template)
{
	gdl_list_t cgroup, csort;
	char *tmp_group, *tmp_sort;
	memset(&cgroup, 0, sizeof(cgroup));
	memset(&csort, 0, sizeof(csort));
	tmp_group = templ_compile(&cgroup, group_template);
	tmp_sort = templ_compile(&csort, sort_template);

	PCB_SUBC_LOOP(data);
	{
		char *grp, *srt;
		grp = templ_exec(subc, &cgroup);
		srt = templ_exec(subc, &csort);
		printf("grp='%s' srt='%s'\n", grp, srt);
	}
	PCB_END_LOOP;


	templ_free(tmp_group, &cgroup);
	templ_free(tmp_sort, &csort);
}


static const char pcb_acts_asm[] = "asm()";
static const char pcb_acth_asm[] = "Interactive assembly assistant";
fgw_error_t pcb_act_asm(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	asm_extract(PCB->Data, group_template, sort_template);
	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t asm_action_list[] = {
	{"asm", pcb_act_asm, pcb_acth_asm, pcb_acts_asm}
};

PCB_REGISTER_ACTIONS(asm_action_list, asm_cookie)

int pplg_check_ver_asm(int ver_needed) { return 0; }

void pplg_uninit_asm(void)
{
	pcb_remove_actions_by_cookie(asm_cookie);
}


#include "dolists.h"
int pplg_init_asm(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(asm_action_list, asm_cookie)

	return 0;
}
