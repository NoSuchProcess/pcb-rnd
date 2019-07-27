/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
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

#include <liblihata/lihata.h>
#include <liblihata/tree.h>
#include "genht/hash.h"
#include "genht/htsp.h"

#include "data.h"
#include "board.h"
#include "conf.h"
#include "error.h"
#include "actions.h"
#include "hid_cfg.h"
#include "compat_misc.h"

#include "menu_helper.h"

int pcb_hid_get_flag(const char *name)
{
	const char *cp;

	if (name == NULL)
		return -1;

	cp = strchr(name, '(');
	if (cp == NULL) {
		conf_native_t *n = pcb_conf_get_field(name);
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

lht_node_t *pcb_hid_cfg_menu_field(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field, const char **field_name)
{
	lht_err_t err;
	const char *fieldstr = NULL;

	switch(field) {
		case PCB_MF_ACCELERATOR:  fieldstr = "a"; break;
		case PCB_MF_SUBMENU:      fieldstr = "submenu"; break;
		case PCB_MF_CHECKED:      fieldstr = "checked"; break;
		case PCB_MF_UPDATE_ON:    fieldstr = "update_on"; break;
		case PCB_MF_SENSITIVE:    fieldstr = "sensitive"; break;
		case PCB_MF_TIP:          fieldstr = "tip"; break;
		case PCB_MF_ACTIVE:       fieldstr = "active"; break;
		case PCB_MF_ACTION:       fieldstr = "action"; break;
		case PCB_MF_FOREGROUND:   fieldstr = "foreground"; break;
		case PCB_MF_BACKGROUND:   fieldstr = "background"; break;
		case PCB_MF_FONT:         fieldstr = "font"; break;
	}
	if (field_name != NULL)
		*field_name = fieldstr;

	if (fieldstr == NULL)
		return NULL;

	return lht_tree_path_(submenu->doc, submenu, fieldstr, 1, 0, &err);
}

const char *pcb_hid_cfg_menu_field_str(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field)
{
	const char *fldname;
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, field, &fldname);

	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		pcb_hid_cfg_error(submenu, "Error: field %s should be a text node\n", fldname);
		return NULL;
	}
	return n->data.text.value;
}

int pcb_hid_cfg_has_submenus(const lht_node_t *submenu)
{
	const char *fldname;
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, PCB_MF_SUBMENU, &fldname);
	if (n == NULL)
		return 0;
	if (n->type != LHT_LIST) {
		pcb_hid_cfg_error(submenu, "Error: field %s should be a list (of submenus)\n", fldname);
		return 0;
	}
	return 1;
}

lht_node_t *pcb_hid_cfg_menu_field_path(const lht_node_t *parent, const char *path)
{
	return lht_tree_path_(parent->doc, parent, path, 1, 0, NULL);
}

static int hid_cfg_remove_item(pcb_hid_cfg_t *hr, lht_node_t *item, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if (gui_remove(ctx, item) != 0)
		return -1;
	lht_tree_del(item);
	return 0;
}

int pcb_hid_cfg_remove_menu_node(pcb_hid_cfg_t *hr, lht_node_t *root, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if ((root == NULL) || (hr == NULL))
		return -1;

	if (root->type == LHT_HASH) {
		lht_node_t *psub, *n, *next;
		psub = pcb_hid_cfg_menu_field(root, PCB_MF_SUBMENU, NULL);
		if (psub != NULL) { /* remove a whole submenu with all children */
			int res = 0;
			for(n = psub->data.list.first; n != NULL; n = next) {
				next = n->next;
				if (pcb_hid_cfg_remove_menu_node(hr, n, gui_remove, ctx) != 0)
					res = -1;
			}
			if (res == 0)
				res = hid_cfg_remove_item(hr, root, gui_remove, ctx);
			return res;
		}
	}

	if ((root->type != LHT_TEXT) && (root->type != LHT_HASH)) /* allow text for the sep */
		return -1;

	/* remove a simple menu item */
	return hid_cfg_remove_item(hr, root, gui_remove, ctx);
}

int pcb_hid_cfg_remove_menu_cookie(pcb_hid_cfg_t *hr, const char *cookie, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx, int level, lht_node_t *curr)
{
	lht_err_t err;
	lht_dom_iterator_t it;

	if ((curr != hr->doc->root) && (level > 1)) {
		curr = lht_tree_path_(hr->doc, curr, "submenu", 1, 0, &err);
		if (curr == NULL)
			return 0;
	}

	for(curr = lht_dom_first(&it, curr); curr != NULL; curr = lht_dom_next(&it)) {
		lht_node_t *nck = lht_tree_path_(hr->doc, curr, "cookie", 1, 0, &err);

		if ((nck != NULL) && (strcmp(nck->data.text.value, cookie) == 0))
			pcb_hid_cfg_remove_menu_node(hr, curr, gui_remove, ctx);
		else
			pcb_hid_cfg_remove_menu_cookie(hr, cookie, gui_remove, ctx, level+1, curr);
	}

	/* always success: not finding any menu is not considered an error */
	return 0;
}

int pcb_hid_cfg_remove_menu(pcb_hid_cfg_t *hr, const char *path, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if (hr != NULL) {
		lht_node_t *nd = pcb_hid_cfg_get_menu_at(hr, NULL, path, NULL, NULL);

		if (nd == NULL)
			return pcb_hid_cfg_remove_menu_cookie(hr, path, gui_remove, ctx, 0, hr->doc->root);
		else
			return pcb_hid_cfg_remove_menu_node(hr, nd, gui_remove, ctx);
	}
	return 0;
}

typedef struct {
	pcb_hid_cfg_t *hr;
	pcb_create_menu_widget_t cb;
	void *cb_ctx;
	lht_node_t *parent;
	const pcb_menu_prop_t *props;
	int target_level;
	int err;
	lht_node_t *after;
} create_menu_ctx_t;

static lht_node_t *create_menu_cb(void *ctx, lht_node_t *node, const char *path, int rel_level)
{
	create_menu_ctx_t *cmc = ctx;
	lht_node_t *psub, *after;

/*	printf(" CB: '%s' %p at %d->%d\n", path, node, rel_level, cmc->target_level);*/
	if (node == NULL) { /* level does not exist, create it */
		const char *name;
		name = strrchr(path, '/');
		if (name != NULL)
			name++;
		else
			name = path;

		if (rel_level <= 1) {
			/* creating a main menu */
			char *end, *name = pcb_strdup(path);
			for(end = name; *end == '/'; end++) ;
			end = strchr(end, '/');
			*end = '\0';
			psub = cmc->parent = pcb_hid_cfg_get_menu(cmc->hr, name);
			free(name);
		}
		else
			psub = pcb_hid_cfg_menu_field(cmc->parent, PCB_MF_SUBMENU, NULL);

		if (rel_level == cmc->target_level) {
			node = pcb_hid_cfg_create_hash_node(psub, cmc->after, name, "dyn", "1", "cookie", cmc->props->cookie, "a", cmc->props->accel, "tip", cmc->props->tip, "action", cmc->props->action, "checked", cmc->props->checked, "update_on", cmc->props->update_on, "foreground", cmc->props->foreground, "background", cmc->props->background, NULL);
			if (node != NULL)
				cmc->err = 0;
		}
		else
			node = pcb_hid_cfg_create_hash_node(psub, cmc->after, name, "dyn", "1", "cookie", cmc->props->cookie,  NULL);

		if (node == NULL)
			return NULL;

		if ((rel_level != cmc->target_level) || (cmc->props->action == NULL))
			lht_dom_hash_put(node, lht_dom_node_alloc(LHT_LIST, "submenu"));

		if (node->parent == NULL) {
			lht_dom_list_append(psub, node);
		}
		else {
			assert(node->parent == psub);
		}

		if ((cmc->after != NULL) && (cmc->after->parent->parent == cmc->parent))
			after = cmc->after;
		else
			after = NULL;

		if (cmc->cb(cmc->cb_ctx, path, name, (rel_level == 1), cmc->parent, after, node) != 0) {
			cmc->err = -1;
			return NULL;
		}
	}
	else {
		/* existing level */
		if ((node->type == LHT_TEXT) && (node->data.text.value[0] == '@')) {
			cmc->after = node;
			goto skip_parent;
		}
	}
	cmc->parent = node;

	skip_parent:;
	return node;
}

int pcb_hid_cfg_create_menu(pcb_hid_cfg_t *hr, const char *path, const pcb_menu_prop_t *props, pcb_create_menu_widget_t cb, void *cb_ctx)
{
	const char *name;
	create_menu_ctx_t cmc;

	cmc.hr = hr;
	cmc.cb = cb;
	cmc.cb_ctx = cb_ctx;
	cmc.parent = NULL;
	cmc.props = props;
	cmc.err = -1;
	cmc.after = NULL;

	/* Allow creating new nodes only under certain main paths that correspond to menus */
	name = path;
	while(*name == '/') name++;

	if ((strncmp(name, "main_menu/", 10) == 0) || (strncmp(name, "popups/", 7) == 0)) {
		/* calculate target level */
		for(cmc.target_level = 0; *name != '\0'; name++) {
			if (*name == '/') {
				cmc.target_level++;
				while(*name == '/') name++;
				name--;
			}
		}

		/* descend and visit each level, create missing levels */
		pcb_hid_cfg_get_menu_at(hr, NULL, path, create_menu_cb, &cmc);
	}

	return cmc.err;
}
