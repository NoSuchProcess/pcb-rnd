/*! \file <ghid-main-menu.c>
 *  \brief Implementation of GHidMainMenu widget
 *  \par Description
 *  This widget is the main pcb menu.
 */

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
#include "hid_flags.h"
#include "hid_cfg.h"
#include "hid_cfg_action.h"
#include "event.h"

#include "bu_menu.h"
#include "wt_route_style.h"
#include "in_keyboard.h"
#include "wt_accel_label.h"


static int action_counter;

typedef struct {
	GtkWidget *widget;						/* for most uses */
	GtkWidget *destroy;						/* destroy this */
} menu_handle_t;

static menu_handle_t *handle_alloc(GtkWidget * widget, GtkWidget * destroy)
{
	menu_handle_t *m = malloc(sizeof(menu_handle_t));
	m->widget = widget;
	m->destroy = destroy;
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

	gint layer_view_pos;
	gint layer_pick_pos;
	gint route_style_pos;

	GtkMenuShell *layer_view_shell;
	GtkMenuShell *layer_pick_shell;
	GtkMenuShell *route_style_shell;

	GList *actions;

	gint n_layer_views;
	gint n_layer_picks;
	gint n_route_styles;

	GCallback action_cb;
};

struct _GHidMainMenuClass {
	GtkMenuBarClass parent_class;
};

/* TODO: write finalize function */

/* SIGNAL HANDLERS */

/* LHT HANDLER */

void ghid_main_menu_real_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu * menu, GtkMenuShell * shell, lht_node_t * base);

static GtkAction *ghid_add_menu(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu * menu, GtkMenuShell * shell, lht_node_t * sub_res)
{
	const char *tmp_val;
	gchar mnemonic = 0;
	GtkAction *action = NULL;
	char *accel = NULL;
	char *menu_label;
	lht_node_t *n_action = pcb_hid_cfg_menu_field(sub_res, PCB_MF_ACTION, NULL);
	lht_node_t *n_keydesc = pcb_hid_cfg_menu_field(sub_res, PCB_MF_ACCELERATOR, NULL);
	int is_main;

	/* Resolve accelerator and save it */
	if (n_keydesc != NULL) {
		if (n_action != NULL) {
			pcb_hid_cfg_keys_add_by_desc(&ghid_keymap, n_keydesc, n_action, NULL, 0);
			accel = pcb_hid_cfg_keys_gen_accel(&ghid_keymap, n_keydesc, 1, NULL);
		}
		else
			pcb_hid_cfg_error(sub_res, "No action specified for key accel\n");
	}

	is_main = (sub_res->parent->type == LHT_LIST) && (sub_res->parent->name != NULL)
		&& (strcmp(sub_res->parent->name, "main_menu") == 0);

	/* Resolve the mnemonic */
	tmp_val = pcb_hid_cfg_menu_field_str(sub_res, PCB_MF_MNEMONIC);
	if (tmp_val)
		mnemonic = tmp_val[0];

	/* Resolve menu name */
	tmp_val = sub_res->name;


	menu_label = g_strdup(tmp_val);

	if (pcb_hid_cfg_has_submenus(sub_res)) {
		/* SUBMENU */
		GtkWidget *submenu = gtk_menu_new();
		GtkWidget *item = gtk_menu_item_new_with_mnemonic(menu_label);
		GtkWidget *tearoff = gtk_tearoff_menu_item_new();
		lht_node_t *n;

		sub_res->user_data = handle_alloc(submenu, item);

		gtk_menu_shell_append(shell, item);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

		/* add tearoff to menu */
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), tearoff);

		/* recurse on the newly-added submenu; pcb_hid_cfg_has_submenus() makes sure
		   the node format is correct; iterate over the list of submenus and create
		   them recursively. */
		n = pcb_hid_cfg_menu_field(sub_res, PCB_MF_SUBMENU, NULL);
		for (n = n->data.list.first; n != NULL; n = n->next)
			ghid_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(submenu), n);
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
/*				pcb_trace("conf_hid_set for %s -> %s\n", checked, nat->hash_path);*/
				conf_hid_set_cb(nat, ctx->ghid_menuconf_id, &cbs);
			}
			else {
				if ((update_on == NULL) || (*update_on != '\0'))	/* warn if update_on is not explicitly empty */
					pcb_message(PCB_MSG_WARNING,
											"Checkbox menu item %s not updated on any conf change - try to use the update_on field\n", checked);
			}
		}
		else if (label && strcmp(label, "false") == 0) {
			/* INSENSITIVE ITEM */
			GtkWidget *item = gtk_menu_item_new_with_label(menu_label);
			gtk_widget_set_sensitive(item, FALSE);
			gtk_menu_shell_append(shell, item);
			sub_res->user_data = handle_alloc(item, item);
		}
		else {
			/* NORMAL ITEM */
			GtkWidget *item = pcb_gtk_menu_item_new(menu_label, accel);
			accel = NULL;
			gtk_menu_shell_append(shell, item);
			sub_res->user_data = handle_alloc(item, item);
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
		GtkWidget *item;
		gtk_action_set_accel_group(action, menu->accel_group);
		gtk_action_connect_accelerator(action);
		g_signal_connect(G_OBJECT(action), "activate", menu->action_cb, (gpointer) n_action);
		g_object_set_data(G_OBJECT(action), "resource", (gpointer) sub_res);
		item = gtk_action_create_menu_item(action);
		gtk_menu_shell_append(shell, item);
		menu->actions = g_list_append(menu->actions, action);
		sub_res->user_data = handle_alloc(item, item);
	}

	/* unused accel key - generated, but never stored, time to free it */
	if (accel != NULL)
		free(accel);

	return action;
}

/*! \brief Translate a resource tree into a menu structure
 *
 *  \param [in] menu    The GHidMainMenu widget to be acted on
 *  \param [in] shall   The base menu shell (a menu bar or popup menu)
 *  \param [in] base    The base of the menu item subtree
 * */
void ghid_main_menu_real_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, GtkMenuShell *shell, lht_node_t *base)
{
	switch (base->type) {
	case LHT_HASH:								/* leaf submenu */
		{
			GtkAction *action = NULL;
			action = ghid_add_menu(ctx, menu, shell, base);
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
	case LHT_TEXT:								/* separator */
		{
			GList *children;
			int pos;

			children = gtk_container_get_children(GTK_CONTAINER(shell));
			pos = g_list_length(children);
			g_list_free(children);

			if ((strcmp(base->data.text.value, "sep") == 0) || (strcmp(base->data.text.value, "-") == 0)) {
				GtkWidget *item = gtk_separator_menu_item_new();
				gtk_menu_shell_append(shell, item);
				base->user_data = handle_alloc(item, item);
			}
			else if (strcmp(base->data.text.value, "@layerview") == 0) {
				menu->layer_view_shell = shell;
				menu->layer_view_pos = pos;
			}
			else if (strcmp(base->data.text.value, "@layerpick") == 0) {
				menu->layer_pick_shell = shell;
				menu->layer_pick_pos = pos;
			}
			else if (strcmp(base->data.text.value, "@routestyles") == 0) {
				menu->route_style_shell = shell;
				menu->route_style_pos = pos;
			}
			else
				pcb_hid_cfg_error(base,
													"Unexpected text node; the only text accepted here is sep, -, @layerview, @layerpick and @routestyles");
		}
		break;
	default:
		pcb_hid_cfg_error(base, "Unexpected node type; should be hash (submenu) or text (separator or @special)");
	}
}

/* CONSTRUCTOR */
static void ghid_main_menu_init(GHidMainMenu * mm)
{
	/* Hookup signal handlers */
}

static void ghid_main_menu_class_init(GHidMainMenuClass * klass)
{
}

/* PUBLIC FUNCTIONS */
GType ghid_main_menu_get_type(void)
{
	static GType mm_type = 0;

	if (!mm_type) {
		const GTypeInfo mm_info = {
			sizeof(GHidMainMenuClass),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_main_menu_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(GHidMainMenu),
			0,												/* n_preallocs */
			(GInstanceInitFunc) ghid_main_menu_init,
		};

		mm_type = g_type_register_static(GTK_TYPE_MENU_BAR, "GHidMainMenu", &mm_info, 0);
	}

	return mm_type;
}

/*! \brief Create a new GHidMainMenu
 *
 *  \return a freshly-allocated GHidMainMenu
 */
GtkWidget *ghid_main_menu_new(GCallback action_cb)
{
	GHidMainMenu *mm = g_object_new(GHID_MAIN_MENU_TYPE, NULL);

	mm->accel_group = gtk_accel_group_new();

	mm->layer_view_pos = 0;
	mm->layer_pick_pos = 0;
	mm->route_style_pos = 0;
	mm->n_layer_views = 0;
	mm->n_layer_picks = 0;
	mm->n_route_styles = 0;
	mm->layer_view_shell = NULL;
	mm->layer_pick_shell = NULL;
	mm->route_style_shell = NULL;

	mm->action_cb = action_cb;
	mm->actions = NULL;

	return GTK_WIDGET(mm);
}

/*! \brief Turn a lht node into the main menu */
void ghid_main_menu_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, const lht_node_t *base)
{
	lht_node_t *n;
	if (base->type != LHT_LIST) {
		pcb_hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}
	for (n = base->data.list.first; n != NULL; n = n->next) {
		ghid_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(menu), n);
	}
}

/*! \brief Turn a lihata node into a popup menu */
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
	base->user_data = handle_alloc(new_menu, new_menu);

	for (i = submenu->data.list.first; i != NULL; i = i->next)
		ghid_main_menu_real_add_node(ctx, menu, GTK_MENU_SHELL(new_menu), i);

	gtk_widget_show_all(new_menu);
}

/*! \brief Updates the toggle/active state of all items
 *  \par Function Description
 *  Loops through all actions, passing the action, its toggle
 *  flag (maybe NULL), and its active flag (maybe NULL), to a
 *  callback function. It is the responsibility of the function
 *  to actually change the state of the action.
 *
 *  \param [in] menu    The menu to be acted on.
 *  \param [in] cb      The callback that toggles the actions
 */
void
ghid_main_menu_update_toggle_state(GHidMainMenu * menu,
																	 void (*cb) (GtkAction *, const char *toggle_flag, const char *active_flag))
{
	GList *list;
	for (list = menu->actions; list; list = list->next) {
		lht_node_t *res = g_object_get_data(G_OBJECT(list->data), "resource");
		lht_node_t *act = pcb_hid_cfg_menu_field(res, PCB_MF_ACTION, NULL);
		const char *tf = g_object_get_data(G_OBJECT(list->data),
																			 "checked-flag");
		const char *af = g_object_get_data(G_OBJECT(list->data),
																			 "active-flag");
		g_signal_handlers_block_by_func(G_OBJECT(list->data), menu->action_cb, act);
		cb(GTK_ACTION(list->data), tf, af);
		g_signal_handlers_unblock_by_func(G_OBJECT(list->data), menu->action_cb, act);
	}
}

/*! \brief Installs or updates layer selector items */
void ghid_main_menu_install_layer_selector(GHidMainMenu * mm)
{
	GList *children, *iter;

	/* @layerview */
	if (mm->layer_view_shell) {
		/* Remove old children */
		children = gtk_container_get_children(GTK_CONTAINER(mm->layer_view_shell));
		for (iter = g_list_nth(children, mm->layer_view_pos);
				 iter != NULL && mm->n_layer_views > 0; iter = g_list_next(iter), mm->n_layer_views--)
			gtk_container_remove(GTK_CONTAINER(mm->layer_view_shell), iter->data);
		g_list_free(children);

		/* Install new ones */
#warning layersel TODO
		mm->n_layer_views = /*pcb_gtk_layer_selector_install_view_items(ls, mm->layer_view_shell, mm->layer_view_pos)*/ NULL;
	}

	/* @layerpick */
	if (mm->layer_pick_shell) {
		/* Remove old children */
		children = gtk_container_get_children(GTK_CONTAINER(mm->layer_pick_shell));
		for (iter = g_list_nth(children, mm->layer_pick_pos);
				 iter != NULL && mm->n_layer_picks > 0; iter = g_list_next(iter), mm->n_layer_picks--)
			gtk_container_remove(GTK_CONTAINER(mm->layer_pick_shell), iter->data);
		g_list_free(children);

		/* Install new ones */
#warning layersel TODO
		mm->n_layer_picks = /*pcb_gtk_layer_selector_install_pick_items(ls, mm->layer_pick_shell, mm->layer_pick_pos);*/ NULL;
	}
}

/*! \brief Installs or updates route style selector items */
void ghid_main_menu_install_route_style_selector(GHidMainMenu * mm, pcb_gtk_route_style_t * rss)
{
	GList *children, *iter;
	/* @routestyles */
	if (mm->route_style_shell) {
		/* Remove old children */
		children = gtk_container_get_children(GTK_CONTAINER(mm->route_style_shell));
		for (iter = g_list_nth(children, mm->route_style_pos);
				 iter != NULL && mm->n_route_styles > 0; iter = g_list_next(iter), mm->n_route_styles--)
			gtk_container_remove(GTK_CONTAINER(mm->route_style_shell), iter->data);
		g_list_free(children);
		/* Install new ones */
		mm->n_route_styles = pcb_gtk_route_style_install_items(rss, mm->route_style_shell, mm->route_style_pos);
	}
}

/*! \brief Returns the menu bar's accelerator group */
GtkAccelGroup *ghid_main_menu_get_accel_group(GHidMainMenu * menu)
{
	if (menu == NULL) {
		pcb_message(PCB_MSG_ERROR, "ghid: can't initialize the menu - is your menu .lht valid?\n");
		exit(1);
	}
	return menu->accel_group;
}

/* Create a new popup window */
static GtkWidget *new_popup(lht_node_t * menu_item)
{
	GtkWidget *new_menu = gtk_menu_new();
/*	GHidMainMenu *menu  = GHID_MAIN_MENU(ctx->menu_bar);*/

	g_object_ref_sink(new_menu);
	menu_item->user_data = handle_alloc(new_menu, new_menu);

	return new_menu;
}

/* Menu widget create callback: create a main menu, popup or submenu as descending the path */
int ghid_create_menu_widget(void *ctx_, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *menu_item)
{
	pcb_gtk_menu_ctx_t *ctx = ctx_;
	int is_popup = (strncmp(path, "/popups", 7) == 0);
	menu_handle_t *ph = parent->user_data;
	GtkWidget *w = (is_main) ? (is_popup ? new_popup(menu_item) : ctx->menu_bar) : ph->widget;

printf("MENU: '%s' %d\n", path, is_popup);;

	ghid_main_menu_real_add_node(ctx, GHID_MAIN_MENU(ctx->menu_bar), GTK_MENU_SHELL(w), menu_item);

/* make sure new menu items appear on screen */
	gtk_widget_show_all(w);
	return 0;
}

int ghid_remove_menu_widget(void *ctx, lht_node_t * nd)
{
	menu_handle_t *h = nd->user_data;
	if (h != NULL) {
/*		printf("GUI remove '%s' %p %p\n", nd->name, h->widget, h->destroy);*/
		gtk_widget_destroy(h->destroy);
		free(h);
		nd->user_data = NULL;
	}
#if 0
	else {
		/* @layer pick and friends */
		printf("GUI remove NULL '%s'\n", nd->name);
	}
#endif
	return 0;
}

/*! \brief callback for ghid_main_menu_update_toggle_state () */
void menu_toggle_update_cb(GtkAction * act, const char *tflag, const char *aflag)
{
	if (tflag != NULL) {
		int v = pcb_hid_get_flag(tflag);
		if (v < 0) {
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), 0);
			gtk_action_set_sensitive(act, 0);
		}
		else
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), ! !v);
	}
	if (aflag != NULL) {
		int v = pcb_hid_get_flag(aflag);
		gtk_action_set_sensitive(act, ! !v);
	}
}

/*! \brief Menu action callback function
 *  \par Function Description
 *  This is the main menu callback function.  The callback receives
 *  the original lihata action node pointer HID actions to be
 *  executed.
 *
 *  \param [in]   The action that was activated
 *  \param [in]   The related menu lht action node
 */

static void ghid_menu_cb(GtkAction * action, const lht_node_t * node)
{
	if (action == NULL || node == NULL)
		return;

	pcb_hid_cfg_action(node);

	pcb_event(PCB_EVENT_GUI_SYNC, NULL);
}


GtkWidget *ghid_load_menus(pcb_gtk_menu_ctx_t *menu, pcb_hid_cfg_t **cfg_out)
{
	const lht_node_t *mr;
	GtkWidget *menu_bar = NULL;
	extern const char *hid_gtk_menu_default;

	*cfg_out = pcb_hid_cfg_load("gtk", 0, hid_gtk_menu_default);
	if (*cfg_out == NULL) {
		pcb_message(PCB_MSG_ERROR, "FATAL: can't load the gtk menu res either from file or from hardwired default.");
		abort();
	}

	mr = pcb_hid_cfg_get_menu(*cfg_out, "/main_menu");
	if (mr != NULL) {
		menu_bar = ghid_main_menu_new(G_CALLBACK(ghid_menu_cb));
		ghid_main_menu_add_node(menu, GHID_MAIN_MENU(menu_bar), mr);
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
	}

	mr = pcb_hid_cfg_get_menu(*cfg_out, "/mouse");
	if (hid_cfg_mouse_init(*cfg_out, &ghid_mouse) != 0)
		pcb_message(PCB_MSG_ERROR, "Error: failed to load mouse actions from the hid config lihata - mouse input will not work.");

	return menu_bar;
}
