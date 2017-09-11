/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"
#include "const.h"
#include "obj_subc.h"
#include "search.h"

static const char *lb_vals[] = { "foo", "bar", "baz", NULL };
static const char *lb_comp[] = { "positive", "negative", NULL };
static const char *lb_types[] = { "paste", "mask", "silk", "copper", "outline", NULL };
static const char *lb_side[] = { "top copper", "bottom copper", NULL };

typedef struct {
	int name, comp, type, dist, side, layer;
} lb_widx_t;

typedef struct {
	const char **layer_names;
	lb_widx_t *widx;
} lb_ctx_t;

static const char pcb_acts_LayerBinding[] = "LayerBinding(object)\nLayerBinding(selected)\nLayerBinding(buffer)\n";
static const char pcb_acth_LayerBinding[] = "Change the layer binding.";
static int pcb_act_LayerBinding(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_data_t *data = NULL;
	pcb_subc_t *subc = NULL;

	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		int type;
		void *ptr1, *ptr2, *ptr3;
		pcb_gui->get_coords("Click on object to change size of", &x, &y);
		type = pcb_search_screen(x, y, PCB_TYPE_SUBC, &ptr1, &ptr2, &ptr3);
		if (type != PCB_TYPE_SUBC) {
			pcb_message(PCB_MSG_ERROR, "No subc under the cursor\n");
			return -1;
		}
		subc = ptr2;
		data = subc->data;
	}
	else if (pcb_strcasecmp(argv[0], "selected") == 0) {
#warning subc TODO
		pcb_message(PCB_MSG_ERROR, "TODO\n");
		return 1;
	}
	else if (pcb_strcasecmp(argv[0], "buffer") == 0) {
#warning subc TODO
		pcb_message(PCB_MSG_ERROR, "TODO\n");
		return 1;
	}
	else
		PCB_ACT_FAIL(LayerBinding);

	{ /* interactive mode */
		int n;
		lb_ctx_t ctx;

		PCB_DAD_DECL(dlg);

		ctx.widx = malloc(sizeof(lb_widx_t) * data->LayerN);
		ctx.layer_names = calloc(sizeof(char *), PCB->Data->LayerN+1);
		for(n = 0; n < PCB->Data->LayerN; n++)
			ctx.layer_names[n] = PCB->Data->Layer[n].meta.real.name;
		ctx.layer_names[n] = NULL;


		PCB_DAD_BEGIN_TABLE(dlg, 2);
		for(n = 0; n < data->LayerN; n++) {
			lb_widx_t *w = ctx.widx+n;
			/* left side */
			PCB_DAD_BEGIN_VBOX(dlg);
				if (n == 0)
					PCB_DAD_LABEL(dlg, "RECIPE");
				else
					PCB_DAD_LABEL(dlg, "\n");

				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_LABEL(dlg, "Name:");
					PCB_DAD_ENUM(dlg, lb_vals);
						w->name = PCB_DAD_CURRENT(dlg);
				PCB_DAD_END(dlg);
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_ENUM(dlg, lb_comp); /* coposite */
						w->comp = PCB_DAD_CURRENT(dlg);
					PCB_DAD_ENUM(dlg, lb_types); /* lyt */
						w->type = PCB_DAD_CURRENT(dlg);
				PCB_DAD_END(dlg);
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_ENUM(dlg, lb_vals);
						w->dist = PCB_DAD_CURRENT(dlg);
					PCB_DAD_LABEL(dlg, "from");
					PCB_DAD_ENUM(dlg, lb_side);
						w->side = PCB_DAD_CURRENT(dlg);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);

			/* right side */
			PCB_DAD_BEGIN_HBOX(dlg);
				PCB_DAD_LABELF(dlg, ("\n\n layer #%d ", n));
				PCB_DAD_BEGIN_VBOX(dlg);
					if (n == 0)
						PCB_DAD_LABEL(dlg, "BOARD LAYER");
					else
						PCB_DAD_LABEL(dlg, "\n\n");
					PCB_DAD_LABEL(dlg, "Automatic");
					PCB_DAD_ENUM(dlg, ctx.layer_names);
						w->layer = PCB_DAD_CURRENT(dlg);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);
		}
		PCB_DAD_END(dlg);

		PCB_DAD_RUN(dlg, "layer_binding", "Layer bindings");

		PCB_DAD_FREE(dlg);
		free(ctx.widx);
		free(ctx.layer_names);
	}

	return 0;
}
