/*
	HOW TO USE THE TEMPLATE:
		1. decide about the name of your dialog box, e.g. foo in this example
		2. svn copy this file to dlg_foo.c - do not use plain fs copy, use svn copy!
		3. rename functions in this file to
		4. add '#include "dlg_foo.c"' in dialogs.c at the end of the list where
		   all the other dlg_*.c files are included
		5. add an action to pcb_act*_Foo in dialogs.c
		6. edit the parts marked with '<<<- edit this' or 'EDIT THIS' - do not
		   yet add the implementation
		7. run 'make dep' in src/
		8. remove this comment block so copyright is on top
		9. edit the copyright message: year and author
		10. test compile
		11. in trunk/: 'svn commit src_plugins/dialogs src/Makefile.dep'
		12. start implementing the dialog box

		This simplistic example is a single-instance non-modal dialog;
		there should be only one global variable, created out of the
		context struct. This ensures it is easy to convert the dialog
		to multi-instance in the future, if needed.
		
		If you need a multi-instance dialog, there should be no global variable
		at all and the context struct should be allocated. A good example
		on this is the shape dialog in ../shape/shape_dialog.c
*/

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "core headers this file depends on (even if other dialogs include them)"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int whatever;
} foo_ctx_t; <<<- rename this

foo_ctx_t foo_ctx;

static void foo_close_cb(void *caller_data, pcb_hid_attr_ev_t ev) <<<- rename this
{
	foo_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(foo_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pcb_dlg_foo(whatever args) <<<- edit this
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (foo_ctx.active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(foo_ctx.dlg);
		PCB_DAD_COMPFLAG(foo_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(foo_ctx.dlg, "foo");
		PCB_DAD_BUTTON_CLOSES(foo_ctx.dlg, clbtn);
	PCB_DAD_END(foo_ctx.dlg);

	/* set up the context */
	foo_ctx.active = 1;

	PCB_DAD_NEW("EDIT_THIS_ID", foo_ctx.dlg, "EDIT THIS: title", &foo_ctx, pcb_false, foo_close_cb);
}

static const char pcb_acts_Foo[] = "Foo(object)\n"; <<<- edit this
static const char pcb_acth_Foo[] = ""; <<<- edit this
static fgw_error_t pcb_act_Foo(fgw_arg_t *res, int argc, fgw_arg_t *argv) <<<- edit this
{
	return 0;
}
