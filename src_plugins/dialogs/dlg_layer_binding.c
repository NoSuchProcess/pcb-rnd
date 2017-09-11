#include "board.h"
#include "data.h"
#include "const.h"
#include "obj_subc.h"
#include "search.h"

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
		const char *vals[] = { "foo", "bar", "baz", NULL };
		const char *comp[] = { "positive", "negative", NULL };
		const char *types[] = { "paste", "mask", "silk", "copper", "outline", NULL };
		const char *side[] = { "top copper", "bottom copper", NULL };
		const char **layer_names;
		PCB_DAD_DECL(dlg);

		layer_names = calloc(sizeof(char *), PCB->Data->LayerN+1);
		for(n = 0; n < PCB->Data->LayerN; n++)
			layer_names[n] = PCB->Data->Layer[n].meta.real.name;
		layer_names[n] = NULL;

		PCB_DAD_BEGIN_TABLE(dlg, 2);
		for(n = 0; n < data->LayerN; n++) {

			/* left side */
			PCB_DAD_BEGIN_VBOX(dlg);
				if (n == 0)
					PCB_DAD_LABEL(dlg, "RECIPE");
				else
					PCB_DAD_LABEL(dlg, "\n");

				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_LABEL(dlg, "Name:");
					PCB_DAD_ENUM(dlg, vals);
				PCB_DAD_END(dlg);
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_ENUM(dlg, comp); /* coposite */
					PCB_DAD_ENUM(dlg, types); /* lyt */
				PCB_DAD_END(dlg);
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_ENUM(dlg, vals);
					PCB_DAD_LABEL(dlg, "from");
					PCB_DAD_ENUM(dlg, side);
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
					PCB_DAD_ENUM(dlg, layer_names);
				PCB_DAD_END(dlg);
			PCB_DAD_END(dlg);
		}
		PCB_DAD_END(dlg);

		PCB_DAD_RUN(dlg, "layer_binding", "Layer bindings");

		PCB_DAD_FREE(dlg);
		free(layer_names);
	}

	return 0;
}
