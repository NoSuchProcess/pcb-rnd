/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"

#include "bu_layer_selector.h"

#include "pcb-printf.h"
#include "pcb_bool.h"
#include "layer.h"
#include "board.h"
#include "data.h"
#include "layer_vis.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "action_helper.h"
#include "compat_misc.h"
#include "bu_menu.h"
#include "glue.h"

static pcb_bool ignore_layer_update;

	/* Silk and rats lines are the two additional selectable to draw on.
	   |  gui code in gui-top-window.c and group code in misc.c must agree
	   |  on what layer is what!
	 */
#define	LAYER_BUTTON_SILK			PCB_MAX_LAYER
#define	LAYER_BUTTON_RATS			(PCB_MAX_LAYER + 1)
#define	N_SELECTABLE_LAYER_BUTTONS	(LAYER_BUTTON_RATS + 1)

#define LAYER_BUTTON_PINS			(PCB_MAX_LAYER + 2)
#define LAYER_BUTTON_VIAS			(PCB_MAX_LAYER + 3)
#define LAYER_BUTTON_FARSIDE		(PCB_MAX_LAYER + 4)
#define LAYER_BUTTON_MASK			(PCB_MAX_LAYER + 5)
#define LAYER_BUTTON_PASTE			(PCB_MAX_LAYER + 6)
#define LAYER_BUTTON_UI			(PCB_MAX_LAYER + 7)
#define N_LAYER_BUTTONS				(PCB_MAX_LAYER + 8)


/* ---------------------------------------------------------------------------
 *
 * layer_process()
 *
 * Takes the index into the layers and produces the text string for
 * the layer and if the layer is currently visible or not.  This is
 * used by a couple of functions.
 *
 */
void layer_process(const gchar **color_string, const char **text, int *set, int i)
{
	int tmp;
	const char *tmps;
	const gchar *tmpc;
	pcb_layer_t *l;

	/* cheap hack to let users pass in NULL for either text or set if
	 * they don't care about the result
	 */

	if (color_string == NULL)
		color_string = &tmpc;

	if (text == NULL)
		text = &tmps;

	if (set == NULL)
		set = &tmp;

	switch (i) {
	case LAYER_BUTTON_SILK:
		*color_string = conf_core.appearance.color.element;
		*text = _("silk");
		*set = PCB->ElementOn;
		break;
	case LAYER_BUTTON_RATS:
		*color_string = conf_core.appearance.color.rat;
		*text = _("rat lines");
		*set = PCB->RatOn;
		break;
	case LAYER_BUTTON_PINS:
		*color_string = conf_core.appearance.color.pin;
		*text = _("pins/pads");
		*set = PCB->PinOn;
		break;
	case LAYER_BUTTON_VIAS:
		*color_string = conf_core.appearance.color.via;
		*text = _("vias");
		*set = PCB->ViaOn;
		break;
	case LAYER_BUTTON_FARSIDE:
		*color_string = conf_core.appearance.color.invisible_objects;
		*text = _("far side");
		*set = PCB->InvisibleObjectsOn;
		break;
	case LAYER_BUTTON_MASK:
		*color_string = conf_core.appearance.color.mask;
		*text = _("solder mask");
		*set = conf_core.editor.show_mask;
		break;
	case LAYER_BUTTON_PASTE:
		*color_string = conf_core.appearance.color.paste;
		*text = _("solder paste");
		*set = conf_core.editor.show_paste;
		break;
	case LAYER_BUTTON_UI:
	default:											/* layers */
		if (i >= LAYER_BUTTON_UI) {
			i -= LAYER_BUTTON_UI;
			l = pcb_get_layer(PCB_LYT_UI + i);
			*color_string = l->Color;
			*text = l->Name;
			*set = l->On;
		}
		else {
			/* plain old copper layer */
			*color_string = conf_core.appearance.color.layer[i];
			*text = (char *) PCB_UNKNOWN(PCB->Data->Layer[i].Name);
			*set = PCB->Data->Layer[i].On;
		}
		break;
	}
}

/*! \brief Callback for pcb_gtk_layer_selector_t layer selection */
void layer_selector_select_callback(pcb_gtk_layer_selector_t *ls, int layer, gpointer data)
{
	pcb_gtk_common_t *com = data;

	ignore_layer_update = pcb_true;
	/* Select Layer */
	PCB->SilkActive = (layer == LAYER_BUTTON_SILK);
	PCB->RatDraw = (layer == LAYER_BUTTON_RATS);
	if (layer == LAYER_BUTTON_SILK) {
		PCB->ElementOn = pcb_true;
		com->LayersChanged();
	}
	else if (layer == LAYER_BUTTON_RATS) {
		PCB->RatOn = pcb_true;
		com->LayersChanged();
	}
	else
		pcb_layervis_change_group_vis(layer, TRUE, pcb_true);

	ignore_layer_update = pcb_false;

	com->invalidate_all();
}

/*! \brief Callback for pcb_gtk_layer_selector_t layer toggling */
void layer_selector_toggle_callback(pcb_gtk_layer_selector_t * ls, int layer, gpointer data)
{
	pcb_gtk_common_t *com = data;
	gboolean redraw = FALSE;
	gboolean active;
	layer_process(NULL, NULL, &active, layer);

	active = !active;
	ignore_layer_update = pcb_true;
	switch (layer) {
	case LAYER_BUTTON_SILK:
		PCB->ElementOn = active;
		PCB->Data->SILKLAYER.On = PCB->ElementOn;
		PCB->Data->BACKSILKLAYER.On = PCB->ElementOn;
		redraw = 1;
		break;
	case LAYER_BUTTON_RATS:
		PCB->RatOn = active;
		redraw = 1;
		break;
	case LAYER_BUTTON_PINS:
		PCB->PinOn = active;
		redraw |= (elementlist_length(&PCB->Data->Element) != 0);
		break;
	case LAYER_BUTTON_VIAS:
		PCB->ViaOn = active;
		redraw |= (pinlist_length(&PCB->Data->Via) != 0);
		break;
	case LAYER_BUTTON_FARSIDE:
		PCB->InvisibleObjectsOn = active;
		PCB->Data->BACKSILKLAYER.On = (active && PCB->ElementOn);
		redraw = TRUE;
		break;
	case LAYER_BUTTON_MASK:
		if (active)
			conf_set_editor(show_mask, 1);
		else
			conf_set_editor(show_mask, 0);
		redraw = TRUE;
		break;
	case LAYER_BUTTON_PASTE:
		if (active)
			conf_set_editor(show_paste, 1);
		else
			conf_set_editor(show_paste, 0);
		redraw = TRUE;
		break;
	default:
		/* Flip the visibility */
		if (layer >= LAYER_BUTTON_UI) {
			layer -= LAYER_BUTTON_UI;
			layer |= PCB_LYT_UI;
		}

		pcb_layervis_change_group_vis(layer, active, pcb_false);
		redraw = TRUE;
		break;
	}

	/* Select the next visible layer. (If there is none, this will
	 * select the currently-selected layer, triggering the selection
	 * callback, which will turn the visibility on.) This way we
	 * will never have an invisible layer selected.
	 */
	if (!active)
		pcb_gtk_layer_selector_select_next_visible(ls);

	ignore_layer_update = pcb_false;

	if (redraw)
		com->invalidate_all();
}


/* \brief Add "virtual layers" to a layer selector */
void make_virtual_layer_buttons(GtkWidget *layer_selector)
{
	pcb_gtk_layer_selector_t *layersel = GHID_LAYER_SELECTOR(layer_selector);
	const gchar *text;
	const gchar *color_string;
	gboolean active;
	pcb_layer_id_t ui[16], numui, n;

	layer_process(&color_string, &text, &active, LAYER_BUTTON_SILK);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_SILK, text, color_string, active, TRUE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_RATS);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_RATS, text, color_string, active, TRUE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_PINS);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_PINS, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_VIAS);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_VIAS, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_FARSIDE);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_FARSIDE, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_MASK);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_MASK, text, color_string, active, FALSE);
	layer_process(&color_string, &text, &active, LAYER_BUTTON_PASTE);
	pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_PASTE, text, color_string, active, FALSE);

	numui = pcb_layer_list(PCB_LYT_UI, ui, sizeof(ui) / sizeof(ui[0]));
	for (n = 0; n < numui; n++) {
		pcb_layer_t *l = pcb_get_layer(ui[n]);
		pcb_gtk_layer_selector_add_layer(layersel, LAYER_BUTTON_UI, l->Name, l->Color, 1, FALSE);
	}
}

/*! \brief Populate a layer selector with all layers Gtk is aware of */
void make_layer_buttons(GtkWidget * layersel)
{
	gint i;
	const gchar *text;
	const gchar *color_string;
	gboolean active = TRUE;

	for (i = 0; i < pcb_max_layer; ++i) {
		if ((pcb_layer_flags(PCB, i) & PCB_LYT_SILK) && !pcb_draw_layer_is_comp(i))
			continue; /* silks have a special, common button - if it's not a composite layer group, skip showing each layer for now */
		layer_process(&color_string, &text, &active, i);
		pcb_gtk_layer_selector_add_layer(GHID_LAYER_SELECTOR(layersel), i, text, color_string, active, TRUE);
	}
}

const char selectlayer_syntax[] = "SelectLayer(1..MAXLAYER|Silk|Rats)";
const char selectlayer_help[] = "Select which layer is the current layer.";

/* %start-doc actions SelectLayer

The specified layer becomes the currently active layer.  It is made
visible if it is not already visible

%end-doc */

int pcb_gtk_SelectLayer(GtkWidget *layer_selector, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int newl;
	if (argc == 0)
		PCB_AFAIL(selectlayer);

	if (pcb_strcasecmp(argv[0], "silk") == 0)
		newl = LAYER_BUTTON_SILK;
	else if (pcb_strcasecmp(argv[0], "rats") == 0)
		newl = LAYER_BUTTON_RATS;
	else
		newl = atoi(argv[0]) - 1;

#ifdef DEBUG_MENUS
	printf("SelectLayer():  newl = %d\n", newl);
#endif

	/* Now that we've figured out which radio button ought to select
	 * this layer, simply hit the button and let the pre-existing code deal
	 */
	pcb_gtk_layer_selector_select_layer(GHID_LAYER_SELECTOR(layer_selector), newl);

	return 0;
}


const char toggleview_syntax[] = "ToggleView(1..MAXLAYER)\n" "ToggleView(layername)\n" "ToggleView(Silk|Rats|Pins|Vias|Mask|BackSide)";
const char toggleview_help[] = "Toggle the visibility of the specified layer or layer group.";

/* %start-doc actions ToggleView

If you pass an integer, that layer is specified by index (the first
layer is @code{1}, etc).  If you pass a layer name, that layer is
specified by name.  When a layer is specified, the visibility of the
layer group containing that layer is toggled.

If you pass a special layer name, the visibility of those components
(silk, rats, etc) is toggled.  Note that if you have a layer named
the same as a special layer, the layer is chosen over the special layer.

%end-doc */

int pcb_gtk_ToggleView(GtkWidget *layer_selector, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_cardinal_t i;
	int l;

#ifdef DEBUG_MENUS
	puts("Starting ToggleView().");
#endif

	if (argc == 0) {
		PCB_AFAIL(toggleview);
	}
	if (isdigit((int) argv[0][0])) {
		l = atoi(argv[0]) - 1;
	}
	else if (strcmp(argv[0], "Silk") == 0)
		l = LAYER_BUTTON_SILK;
	else if (strcmp(argv[0], "Rats") == 0)
		l = LAYER_BUTTON_RATS;
	else if (strcmp(argv[0], "Pins") == 0)
		l = LAYER_BUTTON_PINS;
	else if (strcmp(argv[0], "Vias") == 0)
		l = LAYER_BUTTON_VIAS;
	else if (strcmp(argv[0], "Mask") == 0)
		l = LAYER_BUTTON_MASK;
	else if (strcmp(argv[0], "Paste") == 0)
		l = LAYER_BUTTON_PASTE;
	else if (strcmp(argv[0], "BackSide") == 0)
		l = LAYER_BUTTON_FARSIDE;
	else {
		l = -1;
		for (i = 0; i < pcb_max_layer; i++) {
			if (strcmp(argv[0], PCB->Data->Layer[i].Name) == 0) {
				l = i;
				break;
			}
		}
#warning layer TODO: also check for UI layer names
		if (l == -1)
			PCB_AFAIL(toggleview);
	}

	/* Now that we've figured out which toggle button ought to control
	 * this layer, simply hit the button and let the pre-existing code deal
	 */
	pcb_gtk_layer_selector_toggle_layer(GHID_LAYER_SELECTOR(layer_selector), l);
	return 0;
}

/*! \brief callback for pcb_gtk_layer_selector_delete_layers */
gboolean get_layer_delete(gint layer)
{
	if (pcb_layer_flags(PCB, layer) & PCB_LYT_SILK)
		return 1;
	return layer >= pcb_max_layer;
}

/*! \brief Synchronize layer selector widget with current PCB state
 *  \par Function Description
 *  Called when user toggles layer visibility or changes drawing layer,
 *  or when layer visibility is changed programatically.
 */
void pcb_gtk_layer_buttons_update(GtkWidget *layer_selector, GHidMainMenu *menu)
{
	gint layer;

	if (ignore_layer_update)
		return;

	pcb_gtk_layer_selector_delete_layers(GHID_LAYER_SELECTOR(layer_selector), get_layer_delete);
	make_layer_buttons(layer_selector);
	make_virtual_layer_buttons(layer_selector);
	ghid_main_menu_install_layer_selector(GHID_MAIN_MENU(menu), GHID_LAYER_SELECTOR(layer_selector));

	/* Sync selected layer with PCB's state */
	if (PCB->RatDraw)
		layer = LAYER_BUTTON_RATS;
	else if (PCB->SilkActive)
		layer = LAYER_BUTTON_SILK;
	else
		layer = pcb_layer_stack[0];

	pcb_gtk_layer_selector_select_layer(GHID_LAYER_SELECTOR(layer_selector), layer);
}
