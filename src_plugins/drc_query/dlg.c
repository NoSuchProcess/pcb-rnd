/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <librnd/core/hid_dad.h>

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	conf_role_t role;
	char *rule, *path;
	int wtype, wtitle, wdisable, wdesc, wquery;
} rule_edit_ctx_t;

rule_edit_ctx_t rule_edit_ctx;

static void rule_edit_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	rule_edit_ctx_t *ctx = caller_data;

	free(ctx->path);
	free(ctx->rule);
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static const char *textval(lht_node_t *nd, const char *fname)
{
	lht_node_t *nt = lht_dom_hash_get(nd, fname);

	if ((nt == NULL) || (nt->type != LHT_TEXT))
		return "";

	return nt->data.text.value;
}

static int boolval(lht_node_t *nd, const char *fname)
{
	return str2bool(textval(nd, fname));
}

static void drc_rule_pcb2dlg(rule_edit_ctx_t *ctx)
{
	pcb_dad_retovr_t retovr;
	lht_node_t *nd = pcb_conf_lht_get_at_mainplug(ctx->role, ctx->path, 1, 0);
	if (nd != NULL) {
		pcb_hid_attribute_t *atxt = &ctx->dlg[ctx->wquery];
		pcb_hid_text_t *txt = atxt->wdata;

		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wtype,    str, textval(nd, "type"));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wtitle,   str, textval(nd, "title"));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdisable, str, boolval(nd, "disable"));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdesc,    str, textval(nd, "desc"));

		txt->hid_set_text(atxt, ctx->dlg_hid_ctx, PCB_HID_TEXT_REPLACE, textval(nd, "query"));
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Rule %s disappeared from the config tree.\n", ctx->rule);
		pcb_hid_dad_close(ctx->dlg_hid_ctx, &retovr, -1);
	}
}

static int pcb_dlg_rule_edit(conf_role_t role, const char *rule)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *info, *path;
	rule_edit_ctx_t *ctx;
	lht_node_t *nd;

	path = pcb_concat("plugins/drc_query/rules/", rule, ":0", NULL);
	nd = pcb_conf_lht_get_at_mainplug(role, path, 1, 0);
	if (nd == NULL) {
		pcb_message(PCB_MSG_ERROR, "Rule %s not found on this role.\n", rule);
		return -1;
	}

	ctx = calloc(sizeof(rule_edit_ctx_t), 1);
	ctx->role = role;
	ctx->rule = pcb_strdup(rule);
	ctx->path = path;



	info = pcb_strdup_printf("DRC rule edit: %s on role %s", rule, pcb_conf_role_name(role));
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(ctx->dlg, info);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "DRC violation type (group):");
			PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_WIDTH_CHR(ctx->dlg, 24);
				ctx->wtype = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "DRC violation title:");
			PCB_DAD_STRING(ctx->dlg);
			PCB_DAD_WIDTH_CHR(ctx->dlg, 32);
			ctx->wtitle = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Disable drc rule:");
			PCB_DAD_BOOL(ctx->dlg, "");
			ctx->wdisable = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_LABEL(ctx->dlg, "DRC violation description:");
		PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_WIDTH_CHR(ctx->dlg, 48);
				ctx->wdesc = PCB_DAD_CURRENT(ctx->dlg);

		PCB_DAD_LABEL(ctx->dlg, "DRC rule query script:");
		PCB_DAD_TEXT(ctx->dlg, ctx);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			/*PCB_DAD_CHANGE_CB(ctx->dlg, rule_chg_cb);*/
			ctx->wquery = PCB_DAD_CURRENT(ctx->dlg);


		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Run");
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
	free(info);

	PCB_DAD_DEFSIZE(ctx->dlg, 200, 400);
	PCB_DAD_NEW("drc_query_rule_edit", ctx->dlg, "drc_query: rule editor", ctx, pcb_false, rule_edit_close_cb);

	drc_rule_pcb2dlg(ctx);

	return 0;
}

static const char pcb_acts_DrcQueryEditRule[] = "DrcQueryEditRule(role, path, rule)\nDrcQueryEditRule(role, rule)\n";
static const char pcb_acth_DrcQueryEditRule[] = "Interactive, GUI based DRC rule editor";
static fgw_error_t pcb_act_DrcQueryEditRule(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *srole, *spath, *srule = NULL;
	conf_role_t role;

	PCB_ACT_CONVARG(1, FGW_STR, DrcQueryEditRule, srole = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, DrcQueryEditRule, spath = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, DrcQueryEditRule, srule = argv[3].val.str);

	if (srule == NULL)
		srule = spath;

	role = pcb_conf_role_parse(srole);
	if (role == CFR_invalid)
		PCB_ACT_FAIL(DrcQueryEditRule);

	PCB_ACT_IRES(pcb_dlg_rule_edit(role, srule));
	return 0;
}
