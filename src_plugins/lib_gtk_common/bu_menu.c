#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblihata/tree.h>

#include "board.h"
#include "pcb-printf.h"
#include "misc_util.h"
#include "error.h"
#include "conf.h"
#include "hid_cfg.h"
#include "hid_cfg_action.h"
#include "event.h"

#include "bu_menu.h"
#include "in_keyboard.h"
#include "wt_accel_label.h"

#include "../src_plugins/lib_hid_common/menu_helper.h"

static int action_counter;

typedef struct {
	GtkWidget *widget;    /* for most uses */
	GtkWidget *destroy;   /* destroy this */
	GtkAction *action;    /* for removing from the central lists */
} menu_handle_t;

static menu_handle_t *handle_alloc(GtkWidget *widget, GtkWidget *destroy, GtkAction *action)
{
	menu_handle_t *m = malloc(sizeof(menu_handle_t));
	m->widget = widget;
	m->destroy = destroy;
	m->action = action;
	return m;
}

GtkWidget *pcb_gtk_menu_widget(lht_node_t *node)
{
	menu_handle_t *m;

	if (node == NULL) return NULL;
	if (node->user_data == NULL) return NULL;

	m = node->user_data;
	return m->widget;
}

struct _GHidMainMenu {
	GtkMenuBar parent;
	GtkAccelGroup *accel_group;
	GList *actions;
	GCallback action_cb;
};

struct _GHidMainMenuClass {
	GtkMenuBarClass parent_class;
};

/* LHT HANDLER */

void ghid_main_menu_real_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, GtkMenuShell *shell, lht_node_t *ins_after, lht_node_t *base);

static void ins_menu(GtkWidget *item, GtkMenuShell *shell, lht_node_t *ins_after)
{
	int pos;
	lht_dom_iterator_t it;
	lht_node_t *n;

	/* append at the end of the shell */
	if (ins_after == NULL) {
		gtk_menu_shell_append(shell, item);
		return;
	}

	/* insert after ins_after or append at the end */
	for(n = lht_dom_first(&it, ins_after->parent), pos = 1; n != NULL; n = lht_dom_next(&it),pos++)
		if (n == ins_after)
			break;

	gtk_menu_shell_insert(shell, item, pos);
}

static GtkAction *ghid_add_menu(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, GtkMenuShell *shell, lht_node_t *ins_after, lht_node_t *sub_res)
{
	const char *tmp_val;
	GtkAction *action = NULL;
	char *accel = NULL;
	char *menu_label;
	lht_node_t *n_action = pcb_hid_cfg_menu_field(sub_res, PCB_MF_ACTION, NULL);
	lht_node_t *n_keydesc = pcb_hid_cfg_menu_field(sub_res, PCB_MF_ACCELERATOR, NULL);

	/* Resolve accelerator and save it */
	if (n_keydesc != NULL) {
		if (n_action != NULL) {
			pcb_hid_cfg_keys_add_by_desc(&ghid_keymap, n_keydesc, n_action);
			accel = pcb_hid_cfg_keys_gen_accel(&ghid_keymap, n_keydesc, 1, NULL);
		}
		else
			pcb_hid_cfg_error(sub_res, "No action specified for key accel\n");
	}

	/* Resolve menu name */
	tmp_val = sub_res->name;


	menu_label = g_strdup(tmp_val);

	if (pcb_hid_cfg_has_submenus(sub_res)) {
		/* SUBMENU */
		GtkWidget *submenu = gtk_menu_new();
		GtkWidget *item = gtk_menu_item_new_with_mnemonic(menu_label);
		GtkWidget *tearoff = gtk_tearoff_menu_item_new();
		lht_node_t *n;

		sub_res->user_data = handle_alloc(submenu, item, NULL);
		ins_menu(item, shell, ins_after);

		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

		/* add tearoff to menu */
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), tearoff);

		/* recurse on the newly-added submenu; pcb_hid_cfg_has_submenus() makes sure
		   the node format is correct; iterate over the list of submenus and create
		   them recursively. */
		n = pcb_hid_cfg_menu_field(sub_res, PCB_MF_SUBMENU, NULL);
		for (n = n->data.list.first; n != NULL; n = n->next)
			ghid_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(submenu), NULL, n);
	}
	else {
		/* NON-SUBMENU: MENU ITEM */
		const char *checked = pcb_hid_cfg_menu_field_str(sub_res, PCB_MF_CHECKED);
		const char *update_on = pcb_hid_cfg_menu_field_str(sub_res, PCB_MF_UPDATE_ON);
		const char *label = pcb_hid_cfg_menu_field_str(sub_res, PCB_MF_SENSITIVE);
		const char *tip = pcb_hid_cfg_menu_field_str(sub_res, PCB_MF_TIP);
		if (checked) {
			/* TOGGLE ITEM */
			conf_native_t *nat = NULL;
			gchar *name = g_strdup_printf("MainMenuAction%d", action_counter++);
			action = GTK_ACTION(gtk_toggle_action_new(name, menu_label, tip, NULL));
			/* checked=foo       is a binary flag (checkbox)
			 * checked=foo=bar   is a flag compared to a value (radio) */
			gtk_toggle_action_set_draw_as_radio(GTK_TOGGLE_ACTION(action), ! !strchr(checked, '='));

			if (update_on != NULL)
				nat = conf_get_field(update_on);
			else
				nat = conf_get_field(checked);

			if (nat != NULL) {
				static conf_hid_callbacks_t cbs;
				static int cbs_inited = 0;
				if (!cbs_inited) {
					memset(&cbs, 0, sizeof(conf_hid_callbacks_t));
					cbs.val_change_post = ctx->confchg_checkbox;
					cbs_inited = 1;
				}
				conf_hid_set_cb(nat, ctx->ghid_menuconf_id, &cbs);
			}
			else {
				if ((update_on == NULL) || (*update_on != '\0'))
					pcb_message(PCB_MSG_WARNING, "Checkbox menu item %s not updated on any conf change - try to use the update_on field\n", checked);
			}
		}
		else if (label && strcmp(label, "false") == 0) {
			/* INSENSITIVE ITEM */
			GtkWidget *item = gtk_menu_item_new_with_label(menu_label);
			gtk_widget_set_sensitive(item, FALSE);
			gtk_menu_shell_append(shell, item);
			sub_res->user_data = handle_alloc(item, item, NULL);
		}
		else {
			/* NORMAL ITEM */
			GtkWidget *item = pcb_gtk_menu_item_new(menu_label, accel);
			accel = NULL;
			ins_menu(item, shell, ins_after);
			sub_res->user_data = handle_alloc(item, item, NULL);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu->action_cb), (gpointer) n_action);
			if ((tip != NULL) || (n_keydesc != NULL)) {
				char *acc = NULL, *s;
				if (n_keydesc != NULL)
					acc = pcb_hid_cfg_keys_gen_accel(&ghid_keymap, n_keydesc, -1, "\nhotkey: ");
				s = pcb_concat((tip == NULL ? "" : tip), "\nhotkey: ", (acc == NULL ? "" : acc), NULL);
				gtk_widget_set_tooltip_text(item, s);
				free(s);
			}
		}

	}

	/* By now this runs only for toggle items. */
	if (action) {
		GtkWidget *item = pcb_gtk_checkmenu_item_new(menu_label, accel);

		g_signal_connect(G_OBJECT(action), "activate", menu->action_cb, (gpointer) n_action);
		g_object_set_data(G_OBJECT(action), "resource", (gpointer) sub_res);
		gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE (item), FALSE);
		gtk_activatable_set_related_action(GTK_ACTIVATABLE (item), action);
		ins_menu(item, shell, ins_after);
		menu->actions = g_list_append(menu->actions, action);
		sub_res->user_data = handle_alloc(item, item, action);
	}

	/* unused accel key - generated, but never stored, time to free it */
	if (accel != NULL)
		free(accel);

	return action;
}

/* Translate a resource tree into a menu structure; shell is the base menu
   shell (a menu bar or popup menu) */
void ghid_main_menu_real_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, GtkMenuShell *shell, lht_node_t *ins_after, lht_node_t *base)
{
	switch (base->type) {
	case LHT_HASH:                /* leaf submenu */
		{
			GtkAction *action = NULL;
			action = ghid_add_menu(ctx, menu, shell, ins_after, base);
			if (action) {
				const char *val;

				val = pcb_hid_cfg_menu_field_str(base, PCB_MF_CHECKED);
				if (val != NULL)
					g_object_set_data(G_OBJECT(action), "checked-flag", (gpointer *) val);

				val = pcb_hid_cfg_menu_field_str(base, PCB_MF_ACTIVE);
				if (val != NULL)
					g_object_set_data(G_OBJECT(action), "active-flag", (gpointer *) val);
			}
		}
		break;
	case LHT_TEXT:                /* separator */
		{
			GList *children;

			children = gtk_container_get_children(GTK_CONTAINER(shell));
			g_list_free(children);

			if ((strcmp(base->data.text.value, "sep") == 0) || (strcmp(base->data.text.value, "-") == 0)) {
				GtkWidget *item = gtk_separator_menu_item_new();
				ins_menu(item, shell, ins_after);
				base->user_data = handle_alloc(item, item, NULL);
			}
			else if (base->data.text.value[0] == '@') {
				/* anchor; ignore */
			}
			else
				pcb_hid_cfg_error(base, "Unexpected text node; the only text accepted here is sep, -, or @");
		}
		break;
	default:
		pcb_hid_cfg_error(base, "Unexpected node type; should be hash (submenu) or text (separator or @special)");
	}
}

/* CONSTRUCTOR */
static void ghid_main_menu_init(GHidMainMenu *mm)
{
	/* Hookup signal handlers */
}

static void ghid_main_menu_class_init(GHidMainMenuClass *klass)
{
}

/* PUBLIC FUNCTIONS */
GType ghid_main_menu_get_type(void)
{
	static GType mm_type = 0;

	if (!mm_type) {
		const GTypeInfo mm_info = {
			sizeof(GHidMainMenuClass),
			NULL,                       /* base_init */
			NULL,                       /* base_finalize */
			(GClassInitFunc)ghid_main_menu_class_init,
			NULL,                       /* class_finalize */
			NULL,                       /* class_data */
			sizeof(GHidMainMenu),
			0,                          /* n_preallocs */
			(GInstanceInitFunc) ghid_main_menu_init,
		};

		mm_type = g_type_register_static(GTK_TYPE_MENU_BAR, "GHidMainMenu", &mm_info, 0);
	}

	return mm_type;
}

GtkWidget *ghid_main_menu_new(GCallback action_cb)
{
	GHidMainMenu *mm = g_object_new(GHID_MAIN_MENU_TYPE, NULL);

	mm->accel_group = gtk_accel_group_new();

	mm->action_cb = action_cb;
	mm->actions = NULL;

	return GTK_WIDGET(mm);
}

void ghid_main_menu_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, const lht_node_t *base)
{
	lht_node_t *n;
	if (base->type != LHT_LIST) {
		pcb_hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}
	for (n = base->data.list.first; n != NULL; n = n->next) {
		ghid_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(menu), NULL, n);
	}
}

void ghid_main_menu_add_popup_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, lht_node_t *base)
{
	lht_node_t *submenu, *i;
	GtkWidget *new_menu;

	submenu = pcb_hid_cfg_menu_field_path(base, "submenu");
	if (submenu == NULL) {
		pcb_hid_cfg_error(base, "can not create popup without submenu list");
		return;
	}

	new_menu = gtk_menu_new();
	g_object_ref_sink(new_menu);
	base->user_data = handle_alloc(new_menu, new_menu, NULL);

	for (i = submenu->data.list.first; i != NULL; i = i->next)
		ghid_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(new_menu), NULL, i);

	gtk_widget_show_all(new_menu);
}

/* Updates the toggle/active state of all items:
   Loops through all actions, passing the action, its toggle
   flag (maybe NULL), and its active flag (maybe NULL), to a
   callback function. It is the responsibility of the function
   to actually change the state of the action. */
void ghid_main_menu_update_toggle_state(GHidMainMenu *menu, void (*cb)(GtkAction *, const char *toggle_flag, const char *active_flag))
{
	GList *list;
	for (list = menu->actions; list; list = list->next) {
		lht_node_t *res = g_object_get_data(G_OBJECT(list->data), "resource");
		lht_node_t *act = pcb_hid_cfg_menu_field(res, PCB_MF_ACTION, NULL);
		const char *tf = g_object_get_data(G_OBJECT(list->data), "checked-flag");
		const char *af = g_object_get_data(G_OBJECT(list->data), "active-flag");
		g_signal_handlers_block_by_func(G_OBJECT(list->data), menu->action_cb, act);
		cb(GTK_ACTION(list->data), tf, af);
		g_signal_handlers_unblock_by_func(G_OBJECT(list->data), menu->action_cb, act);
	}
}

GtkAccelGroup *ghid_main_menu_get_accel_group(GHidMainMenu *menu)
{
	if (menu == NULL) {
		pcb_message(PCB_MSG_ERROR, "ghid: can't initialize the menu - is your menu .lht valid?\n");
		exit(1);
	}
	return menu->accel_group;
}

/* Create a new popup window */
static GtkWidget *new_popup(lht_node_t *menu_item)
{
	GtkWidget *new_menu = gtk_menu_new();

	g_object_ref_sink(new_menu);
	menu_item->user_data = handle_alloc(new_menu, new_menu, NULL);

	return new_menu;
}

/* Menu widget create callback: create a main menu, popup or submenu as descending the path */
int ghid_create_menu_widget(void *ctx_, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	pcb_gtk_menu_ctx_t *ctx = ctx_;
	int is_popup = (strncmp(path, "/popups", 7) == 0);
	menu_handle_t *ph = parent->user_data;
	GtkWidget *w = (is_main) ? (is_popup ? new_popup(menu_item) : ctx->menu_bar) : ph->widget;

	ghid_main_menu_real_add_node(ctx, GHID_MAIN_MENU(ctx->menu_bar), GTK_MENU_SHELL(w), ins_after, menu_item);

/* make sure new menu items appear on screen */
	gtk_widget_show_all(w);
	return 0;
}

int ghid_remove_menu_widget(void *ctx, lht_node_t * nd)
{
	menu_handle_t *h = nd->user_data;
	if (h != NULL) {
		GHidMainMenu *menu = (GHidMainMenu *)ctx;
		menu->actions = g_list_remove(menu->actions, h->action);
		gtk_widget_destroy(h->destroy);
		free(h);
		nd->user_data = NULL;
	}
	return 0;
}

/* callback for ghid_main_menu_update_toggle_state() */
void menu_toggle_update_cb(GtkAction *act, const char *tflag, const char *aflag)
{
	if (tflag != NULL) {
		int v = pcb_hid_get_flag(tflag);
		if (v < 0) {
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), 0);
			gtk_action_set_sensitive(act, 0);
		}
		else
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), !!v);
	}
	if (aflag != NULL) {
		int v = pcb_hid_get_flag(aflag);
		gtk_action_set_sensitive(act, !!v);
	}
}

/* Menu action callback function
   This is the main menu callback function.  The callback receives
   the original lihata action node pointer HID actions to be
   executed. */
static void ghid_menu_cb(GtkAction *action, const lht_node_t *node)
{
	pcb_gtk_menu_ctx_t *menu;

	if (action == NULL || node == NULL)
		return;

	menu = node->doc->root->user_data;

	pcb_hid_cfg_action(node);

	pcb_event(menu->hidlib, PCB_EVENT_GUI_SYNC, NULL);
}


GtkWidget *ghid_load_menus(pcb_gtk_menu_ctx_t *menu, pcb_hidlib_t *hidlib, pcb_hid_cfg_t **cfg_out)
{
	const lht_node_t *mr;
	GtkWidget *menu_bar = NULL;
	extern const char *pcb_menu_default;

	menu->hidlib = hidlib;

	*cfg_out = pcb_hid_cfg_load(menu->hidlib, "gtk", 0, pcb_menu_default);
	if (*cfg_out == NULL) {
		pcb_message(PCB_MSG_ERROR, "FATAL: can't load the gtk menu res either from file or from hardwired default.");
		abort();
	}

	mr = pcb_hid_cfg_get_menu(*cfg_out, "/main_menu");
	if (mr != NULL) {
		menu_bar = ghid_main_menu_new(G_CALLBACK(ghid_menu_cb));
		ghid_main_menu_add_node(menu, GHID_MAIN_MENU(menu_bar), mr);
		mr->doc->root->user_data = menu;
	}

	mr = pcb_hid_cfg_get_menu(*cfg_out, "/popups");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;
			for (n = mr->data.list.first; n != NULL; n = n->next)
				ghid_main_menu_add_popup_node(menu, GHID_MAIN_MENU(menu_bar), n);
		}
		else
			pcb_hid_cfg_error(mr, "/popups should be a list");
		mr->doc->root->user_data = menu;
	}

	mr = pcb_hid_cfg_get_menu(*cfg_out, "/mouse");
	if (hid_cfg_mouse_init(*cfg_out, &ghid_mouse) != 0)
		pcb_message(PCB_MSG_ERROR, "Error: failed to load mouse actions from the hid config lihata - mouse input will not work.");

	return menu_bar;
}
