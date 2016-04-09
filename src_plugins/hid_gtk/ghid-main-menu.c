/*! \file <ghid-main-menu.c>
 *  \brief Implementation of GHidMainMenu widget
 *  \par Description
 *  This widget is the main pcb menu.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblihata/tree.h>

#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "ghid-main-menu.h"
#include "ghid-layer-selector.h"
#include "ghid-route-style-selector.h"

void Message(const char *, ...);

static int action_counter;

struct _GHidMainMenu {
	GtkMenuBar parent;

	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;

	gint layer_view_pos;
	gint layer_pick_pos;
	gint route_style_pos;

	GtkMenuShell *layer_view_shell;
	GtkMenuShell *layer_pick_shell;
	GtkMenuShell *route_style_shell;

	GList *actions;
	GHashTable *popup_table;

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

void ghid_main_menu_real_add_node(GHidMainMenu * menu, GtkMenuShell * shell, lht_node_t *base);

static GtkAction *ghid_add_menu(GHidMainMenu * menu, GtkMenuShell * shell, lht_node_t * sub_res)
{
	const char *tmp_val;
	gchar mnemonic = 0;
	int j;
	GtkAction *action = NULL;
	const gchar *accel = NULL;
	char *menu_label;
	lht_node_t *n_action = hid_cfg_menu_field(sub_res, MF_ACTION, NULL);

	/* Resolve accelerator and save it */
	tmp_val = hid_cfg_menu_field_str(sub_res, MF_ACCELERATOR);
	if (tmp_val != NULL) {
		lht_node_t *anode;

		assert(node != NULL);
		anode = hid_cfg_menu_field(sub_res, MF_ACTION, NULL);
		if (anode != NULL)
			hid_cfg_keys_add_by_desc(&ghid_keymap, tmp_val, anode, NULL, 0);
		else
			hid_cfg_error(sub_res, "No action specified for key accel\n");
	}

	/* Resolve the mnemonic */
	tmp_val = hid_cfg_menu_field_str(sub_res, MF_MNEMONIC);
	if (tmp_val)
		mnemonic = tmp_val[0];

	/* Resolve menu name */
	tmp_val = sub_res->name;

	/* Hack '_' in based on mnemonic value */
	if (!mnemonic)
		menu_label = g_strdup(tmp_val);
	else {
		char *post_ = strchr(tmp_val, mnemonic);
		if (post_ == NULL)
			menu_label = g_strdup(tmp_val);
		else {
			GString *tmp = g_string_new("");
			g_string_append_len(tmp, tmp_val, post_ - tmp_val);
			g_string_append_c(tmp, '_');
			g_string_append(tmp, post_);
			menu_label = g_string_free(tmp, FALSE);
		}
	}

	if (hid_cfg_has_submenus(sub_res)) {
		/* SUBMENU */
		GtkWidget *submenu = gtk_menu_new();
		GtkWidget *item = gtk_menu_item_new_with_mnemonic(menu_label);
		GtkWidget *tearoff = gtk_tearoff_menu_item_new();
		lht_node_t *n;

		sub_res->user_data = submenu;

		gtk_menu_shell_append(shell, item);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

		/* add tearoff to menu */
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), tearoff);

		/* recurse on the newly-added submenu; hid_cfg_has_submenus() makes sure
		   the node format is correct; iterate over the list of submenus and create
		   them recursively. */
		n = hid_cfg_menu_field(sub_res, MF_SUBMENU, NULL);
		for(n = n->data.list.first; n != NULL; n = n->next)
			ghid_main_menu_real_add_node(menu, GTK_MENU_SHELL(submenu), n);
	}
	else {
		/* NON-SUBMENU: MENU ITEM */
		const char *checked = hid_cfg_menu_field_str(sub_res, MF_CHECKED);
		const char *label = hid_cfg_menu_field_str(sub_res, MF_SENSITIVE);
		const char *tip = hid_cfg_menu_field_str(sub_res, MF_TIP);
		if (checked) {
			/* TOGGLE ITEM */
			gchar *name = g_strdup_printf("MainMenuAction%d", action_counter++);
			action = GTK_ACTION(gtk_toggle_action_new(name, menu_label, tip, NULL));
			/* checked=foo       is a binary flag (checkbox)
			 * checked=foo,bar   is a flag compared to a value (radio) */
			gtk_toggle_action_set_draw_as_radio(GTK_TOGGLE_ACTION(action), ! !strchr(checked, ','));
		}
		else if (label && strcmp(label, "false") == 0) {
			/* INSENSITIVE ITEM */
			GtkWidget *item = gtk_menu_item_new_with_label(menu_label);
			gtk_widget_set_sensitive(item, FALSE);
			gtk_menu_shell_append(shell, item);
			sub_res->user_data = item;
		}
		else {
			/* NORMAL ITEM */
			gchar *name = g_strdup_printf("MainMenuAction%d", action_counter++);
			action = gtk_action_new(name, menu_label, tip, NULL);
		}
	}

	/* Connect accelerator, if there is one */
	if (action) {
		GtkWidget *item;
		gtk_action_set_accel_group(action, menu->accel_group);
//		gtk_action_group_add_action_with_accel(menu->action_group, action, accel);
		gtk_action_connect_accelerator(action);
		g_signal_connect(G_OBJECT(action), "activate", menu->action_cb, (gpointer) n_action);
		g_object_set_data(G_OBJECT(action), "resource", (gpointer) sub_res);
		item = gtk_action_create_menu_item(action);
		gtk_menu_shell_append(shell, item);
		menu->actions = g_list_append(menu->actions, action);
		sub_res->user_data = item;
	}

	return action;
}

/*! \brief Translate a resource tree into a menu structure
 *
 *  \param [in] menu    The GHidMainMenu widget to be acted on
 *  \param [in] shall   The base menu shell (a menu bar or popup menu)
 *  \param [in] res     The base of the menu item subtree
 * */
void ghid_main_menu_real_add_node(GHidMainMenu * menu, GtkMenuShell * shell, lht_node_t *base)
{
	lht_node_t *n;

	switch(base->type) {
		case LHT_HASH: /* leaf submenu */
			{
				GtkAction *action = NULL;
				action = ghid_add_menu(menu, shell, base);
				if (action) {
					const char *val;

					val = hid_cfg_menu_field_str(base, MF_CHECKED);
					if (val != NULL)
						g_object_set_data(G_OBJECT(action), "checked-flag", (gpointer *)val);

					val = hid_cfg_menu_field_str(base, MF_ACTIVE);
					if (val != NULL)
						g_object_set_data(G_OBJECT(action), "active-flag", (gpointer *)val);
				}
			}
			break;
		case LHT_TEXT: /* separator */
			{
				GList *children;
				int pos;

				children = gtk_container_get_children(GTK_CONTAINER(shell));
				pos = g_list_length(children);
				g_list_free(children);

				if ((strcmp(base->data.text.value, "sep") == 0) || (strcmp(base->data.text.value, "-") == 0)) {
					GtkWidget *item = gtk_separator_menu_item_new();
					gtk_menu_shell_append(shell, item);
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
					hid_cfg_error(base, "Unexpected text node; the only text accepted here is sep, -, @layerview, @layerpick and @routestyles");
			}
			break;
		default:
			hid_cfg_error(base, "Unexpected node type; should be hash (submenu) or text (separator or @special)");
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
	mm->action_group = gtk_action_group_new("MainMenu");

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
	mm->popup_table = g_hash_table_new(g_str_hash, g_str_equal);

	return GTK_WIDGET(mm);
}

/*! \brief Turn a lht node into the main menu */
void ghid_main_menu_add_node(GHidMainMenu * menu, const lht_node_t *base)
{
	lht_node_t *n;
	if (base->type != LHT_LIST) {
		hid_cfg_error(base, "Menu description shall be a list (li)\n");
		abort();
	}
	for(n = base->data.list.first; n != NULL; n = n->next) {
		ghid_main_menu_real_add_node(menu, GTK_MENU_SHELL(menu), n);
	}
}

/*! \brief Turn a lihata node into a popup menu */
void ghid_main_menu_add_popup_node(GHidMainMenu * menu, lht_node_t *base)
{
	lht_node_t *submenu, *i;
	GtkWidget *new_menu;

	submenu = hid_cfg_get_submenu(base, "submenu");
	if (submenu == NULL) {
		hid_cfg_error(base, "can not create popup without submenu list");
		return;
	}

	new_menu = gtk_menu_new();
	g_object_ref_sink(new_menu);
	base->user_data = new_menu;

	for(i = submenu->data.list.first; i != NULL; i = i->next)
		ghid_main_menu_real_add_node(menu, GTK_MENU_SHELL(new_menu), i);

	g_hash_table_insert(menu->popup_table, (gpointer) base->name, new_menu);
	gtk_widget_show_all(new_menu);
}

/*! \brief Returns a registered popup menu by name */
GtkMenu *ghid_main_menu_get_popup(GHidMainMenu * menu, const char *name)
{
	return g_hash_table_lookup(menu->popup_table, name);
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
		lht_node_t *act = hid_cfg_menu_field(res, MF_ACTION, NULL);
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
void ghid_main_menu_install_layer_selector(GHidMainMenu * mm, GHidLayerSelector * ls)
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
		mm->n_layer_views = ghid_layer_selector_install_view_items(ls, mm->layer_view_shell, mm->layer_view_pos);
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
		mm->n_layer_picks = ghid_layer_selector_install_pick_items(ls, mm->layer_pick_shell, mm->layer_pick_pos);
	}
}

/*! \brief Installs or updates route style selector items */
void ghid_main_menu_install_route_style_selector(GHidMainMenu * mm, GHidRouteStyleSelector * rss)
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
		mm->n_route_styles = ghid_route_style_selector_install_items(rss, mm->route_style_shell, mm->route_style_pos);
	}
}

/*! \brief Returns the menu bar's accelerator group */
GtkAccelGroup *ghid_main_menu_get_accel_group(GHidMainMenu * menu)
{
	if (menu == NULL) {
		Message("ghid: can't initialize the menu - is your gpcb-menu.res valid?\n");
		exit(1);
	}
	return menu->accel_group;
}

/* Create a new popup window */
static GtkWidget *new_popup(lht_node_t *menu_item)
{
	GtkWidget *new_menu = gtk_menu_new();;
	GHidMainMenu *menu  = GHID_MAIN_MENU(ghidgui->menu_bar);

	g_object_ref_sink(new_menu);
	menu_item->user_data = new_menu;

	g_hash_table_insert(menu->popup_table, (gpointer) menu_item->name, new_menu);

	return new_menu;
}

/* Menu widget create callback: create a main menu, popup or submenu as descending the path */
static int ghid_create_menu_widget(void *ctx, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *menu_item)
{
	int is_popup = (strncmp(path, "/popups", 7) == 0);
	GtkWidget *w = (is_main) ? (is_popup ? new_popup(menu_item) : ghidgui->menu_bar) : parent->user_data;

	ghid_main_menu_real_add_node(GHID_MAIN_MENU(ghidgui->menu_bar), GTK_MENU_SHELL(w), menu_item);

/* make sure new menu items appear on screen */
	gtk_widget_show_all(w);
	return 0;
}

/* Create a new menu by path */
void ghid_create_menu(const char *menu_path, const char *action, const char *mnemonic, const char *accel, const char *tip)
{
	hid_cfg_create_menu(ghid_cfg, menu_path, action, mnemonic, accel, tip, ghid_create_menu_widget, NULL);
}
