/*! \file <ghid-main-menu.c>
 *  \brief Implementation of GHidMainMenu widget
 *  \par Description
 *  This widget is the main pcb menu.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "ghid-main-menu.h"
#include "ghid-layer-selector.h"
#include "ghid-route-style-selector.h"

void Message (const char *, ...);

static int action_counter;

struct _GHidMainMenu
{
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
  void (*special_key_cb) (const char *accel, GtkAction *action,
                          const Resource *node);
};

struct _GHidMainMenuClass
{
  GtkMenuBarClass parent_class;
};

/* TODO: write finalize function */

/* SIGNAL HANDLERS */

/* RESOURCE HANDLER */
/* \brief Translate gpcb-menu.res accelerators to gtk ones
 * \par Function Description
 * Some keys need to be replaced by a name for the gtk accelerators to
 * work.  This table contains the translations.  The "in" character is
 * what would appear in gpcb-menu.res and the "out" string is what we
 * have to feed to gtk.  I was able to find these by using xev to find
 * the keycode and then looked at gtk+-2.10.9/gdk/keynames.txt (from the
 * gtk source distribution) to figure out the names that go with the 
 * codes.
 */
static gchar *
translate_accelerator (const char *text)
{
  GString *ret_val = g_string_new ("");
  static struct { const char *in, *out; } key_table[] = 
  {
    {"Enter", "Return"},
    {"Alt",   "<alt>"},
    {"Shift", "<shift>"},
    {"Ctrl",  "<ctrl>"},
    {" ", ""},
    {":", "colon"},
    {"=", "equal"},
    {"/", "slash"},
    {"[", "bracketleft"},
    {"]", "bracketright"},
    {".", "period"},
    {"|", "bar"},
    {NULL, NULL}
  };

  enum {MOD, KEY} state = MOD;
  while (*text != '\0')
    {
      static gboolean gave_msg;
      gboolean found = FALSE;
      int i;

      if (state == MOD && strncmp (text, "<Key>", 5) == 0)
        {
          state = KEY;
          text += 5;
        }
      for (i = 0; key_table[i].in != NULL; ++i)
        {
          int len = strlen (key_table[i].in);
          if (strncmp (text, key_table[i].in, len) == 0)
            {
              found = TRUE;
              g_string_append (ret_val, key_table[i].out);
              text += len;
            }
        }
      if (found == FALSE)
        switch (state)
          {
          case MOD:
            Message (_("Don't know how to parse \"%s\" as an "
                       "accelerator in the menu resource file.\n"),
                     text);
            if (!gave_msg)
              {
                gave_msg = TRUE;
                Message (_("Format is:\n"
                           "modifiers<Key>k\n"
                           "where \"modifiers\" is a space "
                           "separated list of key modifiers\n"
                           "and \"k\" is the name of the key.\n"
                           "Allowed modifiers are:\n"
                           "   Ctrl\n"
                           "   Shift\n"
                           "   Alt\n"
                           "Please note that case is important.\n"));
              }
              text++;
            break;
          case KEY:
            g_string_append_c (ret_val, *text);
            ++text;
            break;
          }
    }
  return g_string_free (ret_val, FALSE);
}

/*! \brief Check that translated accelerators are unique; warn otherwise. */
static const char *
check_unique_accel (const char *accelerator)
{
  static GHashTable *accel_table;

  if (!accelerator ||*accelerator)
    return accelerator;

  if (!accel_table)
    accel_table = g_hash_table_new (g_str_hash, g_str_equal);

  if (g_hash_table_lookup (accel_table, accelerator))
    {
       Message (_("Duplicate accelerator found: \"%s\"\n"
                  "The second occurance will be dropped\n"),
                accelerator);
        return NULL;
    }

  g_hash_table_insert (accel_table,
                       (gpointer) accelerator, (gpointer) accelerator);

  return accelerator;
}

void
ghid_main_menu_real_add_resource (GHidMainMenu *menu, GtkMenuShell *shell,
                                  const Resource *res, const char *path);


static GHashTable *menu_hash = NULL; /* path->GtkWidget */

#ifdef DEBUG_GTK
static void print_widget(GtkWidget *w)
{
	fprintf(stderr, " %p %d;%d %d;%d %d;%d\n", w, w->allocation.x, w->allocation.y, w->allocation.width, w->allocation.height, w->requisition.width, w->requisition.height);
	fprintf(stderr, "  flags=%x typ=%d realized=%d vis=%d\n", GTK_WIDGET_FLAGS(w), GTK_WIDGET_TYPE(w), GTK_WIDGET_REALIZED(w), GTK_WIDGET_VISIBLE(w));
}

//#include <glib.h>
static void print_menu_shell_cb(void *obj, void *ud)
{
	GtkWidget *w = obj;
	print_widget(w);
}

static void print_menu_shell(GtkMenuShell *sh)
{
	GList *children;
	children = gtk_container_get_children (GTK_CONTAINER (sh));
	g_list_foreach(children, print_menu_shell_cb, NULL);
}
#endif

static GtkAction *ghid_add_menu(GHidMainMenu *menu, GtkMenuShell *shell,
                                  const Resource *sub_res, const char *path)
{
  const char *res_val;
  const Resource *tmp_res;
  gchar mnemonic = 0;
  int j;
  GtkAction *action = NULL;
  const gchar *accel = NULL;
  char *menu_label;
  char *npath;

  if (!menu_hash)
    menu_hash = g_hash_table_new (g_str_hash, g_str_equal);


          tmp_res = resource_subres (sub_res, "a");  /* accelerator */
          res_val = resource_value (sub_res, "m");   /* mnemonic */



          if (res_val)
            mnemonic = res_val[0];
          /* The accelerator resource will have two values, like 
           *   a={"Ctrl-Q" "Ctrl<Key>q"}
           * The first Gtk ignores. The second needs to be translated. */
          if (tmp_res)
            accel = check_unique_accel
                      (translate_accelerator (tmp_res->v[1].value));

          /* Now look for the first unnamed value (not a subresource) to
           * figure out the name of the menu or the menuitem. */
          res_val = "button";
          for (j = 0; j < sub_res->c; ++j)
            if (resource_type (sub_res->v[j]) == 10)
              {
                res_val = sub_res->v[j].value;
                break;
              }
          /* Hack '_' in based on mnemonic value */
          if (!mnemonic)
            menu_label = g_strdup (res_val);
          else
            {
              char *post_ = strchr (res_val, mnemonic);
              if (post_ == NULL)
                menu_label = g_strdup (res_val);
              else
                {
                  GString *tmp = g_string_new ("");
                  g_string_append_len (tmp, res_val, post_ - res_val);
                  g_string_append_c (tmp, '_');
                  g_string_append (tmp, post_);
                  menu_label = g_string_free (tmp, FALSE);
                }
            }

           npath = Concat(path, "/", res_val, NULL);

          /* If the subresource we're processing also has unnamed
           * subresources, it's a submenu, not a regular menuitem. */
/* fprintf(stderr, "menu path='%s' label='%s' flag=%d\n", npath, menu_label, sub_res->flags); */
          if (sub_res->flags & FLAG_S)
            {
              /* SUBMENU */
              GtkWidget *submenu = gtk_menu_new ();
              GtkWidget *item = gtk_menu_item_new_with_mnemonic (menu_label);
              GtkWidget *tearoff = gtk_tearoff_menu_item_new ();

/*fprintf(stderr, "menu path='%s' SUB submenu=%p\n", npath, submenu);*/

              gtk_menu_shell_append (shell, item);
              gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

              /* add tearoff to menu */
              gtk_menu_shell_append (GTK_MENU_SHELL (submenu), tearoff);

/*
fprintf(stderr, "     hi1 path='%s' item=%p\n", npath, (gpointer)submenu);
fprintf(stderr, "subm1:\n");
print_widget(submenu);
fprintf(stderr, "item1:\n");
print_widget(item);
*/

              g_hash_table_insert (menu_hash, (gpointer)npath, (gpointer)submenu);

              /* recurse on the newly-added submenu */
              ghid_main_menu_real_add_resource (menu,
                                                GTK_MENU_SHELL (submenu),
                                                sub_res, npath);

/*
fprintf(stderr, "subm2:\n");
print_widget(submenu);
fprintf(stderr, "item2:\n");
print_widget(item);
*/

            }
          else
            {
/*fprintf(stderr, "menu path='%s' MAIN\n", npath, res_val);*/
              /* NON-SUBMENU: MENU ITEM */
              const char *checked = resource_value (sub_res, "checked");
              const char *label = resource_value (sub_res, "sensitive");
              const char *tip = resource_value (sub_res, "tip");
              if (checked)
                {
                  /* TOGGLE ITEM */
                  gchar *name = g_strdup_printf ("MainMenuAction%d",
                                                 action_counter++);

                  action = GTK_ACTION (gtk_toggle_action_new (name, menu_label,
                                                              tip, NULL));
                  /* checked=foo       is a binary flag (checkbox)
                   * checked=foo,bar   is a flag compared to a value (radio) */
                  gtk_toggle_action_set_draw_as_radio
                    (GTK_TOGGLE_ACTION (action), !!strchr (checked, ','));
                }
              else if (label && strcmp (label, "false") == 0)
                {
                  /* INSENSITIVE ITEM */
                  GtkWidget *item = gtk_menu_item_new_with_label (menu_label);
                  gtk_widget_set_sensitive (item, FALSE);
                  gtk_menu_shell_append (shell, item);
/*fprintf(stderr, "     hi2 path='%s' item=%p\n", npath, (gpointer)item);*/
                  g_hash_table_insert (menu_hash, (gpointer)npath, (gpointer)item);
                }
              else
                {
                  /* NORMAL ITEM */
                  gchar *name = g_strdup_printf ("MainMenuAction%d", action_counter++);
                  action = gtk_action_new (name, menu_label, tip, NULL);
/*                  fprintf(stderr, " Action! '%s' '%s' '%s'\n", name, menu_label, tip);*/
                }
            }

          /* Connect accelerator, if there is one */
          if (action)
            {
              GtkWidget *item;
              gtk_action_set_accel_group (action, menu->accel_group);
              gtk_action_group_add_action_with_accel (menu->action_group,
                                                      action, accel);
              gtk_action_connect_accelerator (action);
              g_signal_connect (G_OBJECT (action), "activate", menu->action_cb,
                                (gpointer) sub_res);
              g_object_set_data (G_OBJECT (action), "resource",
                                 (gpointer) sub_res);
              item = gtk_action_create_menu_item (action);
              gtk_menu_shell_append (shell, item);
              menu->actions = g_list_append (menu->actions, action);
              menu->special_key_cb (accel, action, sub_res);

/*fprintf(stderr, "     hi3 path='%s' item=%p\n", npath, (gpointer)item);*/
              g_hash_table_insert (menu_hash, (gpointer)npath, (gpointer)item);
            }

/* keep npath for the hash so do not free(npath); */
  return action;
}


/*! \brief Translate a resource tree into a menu structure
 *
 *  \param [in] menu    The GHidMainMenu widget to be acted on
 *  \param [in] shall   The base menu shell (a menu bar or popup menu)
 *  \param [in] res     The base of the resource tree
 * */
void
ghid_main_menu_real_add_resource (GHidMainMenu *menu, GtkMenuShell *shell,
                                  const Resource *res, const char *path)
{
  int i, j;
  GtkAction *action = NULL;

  for (i = 0; i < res->c; ++i)
    {
      const Resource *sub_res = res->v[i].subres;

      switch (resource_type (res->v[i]))
        {
        case 101:   /* name, subres: passthrough */
          ghid_main_menu_real_add_resource (menu, shell, sub_res, path);
          break;
        case   1:   /* no name, subres */
          action = ghid_add_menu(menu, shell, sub_res, path);
          /* Scan rest of resource in case there is more work */
          for (j = 0; j < sub_res->c; j++)
            {
              const char *res_name;
              /* named value = X resource */
              if (resource_type (sub_res->v[j]) == 110)
                {
                  res_name = sub_res->v[j].name;

                  /* translate bg, fg to background, foreground */
                  if (strcmp (res_name, "fg") == 0)   res_name = "foreground";
                  if (strcmp (res_name, "bg") == 0)   res_name = "background";

                  /* ignore special named values (m, a, sensitive) */
                  if (strcmp (res_name, "m") == 0
                      || strcmp (res_name, "a") == 0
                      || strcmp (res_name, "sensitive") == 0
                      || strcmp (res_name, "tip") == 0)
                    break;

                  /* log checked and active special values */
                  if (action && strcmp (res_name, "checked") == 0)
                    g_object_set_data (G_OBJECT (action), "checked-flag",
                                       sub_res->v[j].value);
                  else if (action && strcmp (res_name, "active") == 0)
                    g_object_set_data (G_OBJECT (action), "active-flag",
                                       sub_res->v[j].value);
                  else
                    /* if we got this far it is supposed to be an X
                     * resource.  For now ignore it and warn the user */
                    Message (_("The gtk gui currently ignores \"%s\""
                               "as part of a menuitem resource.\n"
                               "Feel free to provide patches\n"),
                             sub_res->v[j].value);
                }
            }
          break;
        case  10:   /* no name, value */
          /* If we get here, the resource is "-" or "@foo" for some foo */
          if (res->v[i].value[0] == '@')
            {
              GList *children;
              int pos;

              children = gtk_container_get_children (GTK_CONTAINER (shell));
              pos = g_list_length (children);
              g_list_free (children);

              if (strcmp (res->v[i].value, "@layerview") == 0)
                {
                  menu->layer_view_shell = shell;
                  menu->layer_view_pos = pos;
                }
              else if (strcmp (res->v[i].value, "@layerpick") == 0)
                {
                  menu->layer_pick_shell = shell;
                  menu->layer_pick_pos = pos;
                }
              else if (strcmp (res->v[i].value, "@routestyles") == 0)
                {
                  menu->route_style_shell = shell;
                  menu->route_style_pos = pos;
                }
              else
                Message (_("GTK GUI currently ignores \"%s\" in the menu\n"
                           "resource file.\n"), res->v[i].value);
            }
          else if (strcmp (res->v[i].value, "-") == 0)
            {
              GtkWidget *item = gtk_separator_menu_item_new ();
              gtk_menu_shell_append (shell, item);
            }
          else if (i > 0)
            {
              /* This is an action-less menuitem. It is really only useful
               * when you're starting to build a new menu and you're looking
               * to get the layout right. */
              GtkWidget *item
                = gtk_menu_item_new_with_label (res->v[i].value);
              gtk_menu_shell_append (shell, item);
            }
          break;
      }
  }
}

/* CONSTRUCTOR */
static void
ghid_main_menu_init (GHidMainMenu *mm)
{
  /* Hookup signal handlers */
}

static void
ghid_main_menu_class_init (GHidMainMenuClass *klass)
{
}

/* PUBLIC FUNCTIONS */
GType
ghid_main_menu_get_type (void)
{
  static GType mm_type = 0;

  if (!mm_type)
    {
      const GTypeInfo mm_info =
        {
          sizeof (GHidMainMenuClass),
          NULL, /* base_init */
          NULL, /* base_finalize */
          (GClassInitFunc) ghid_main_menu_class_init,
          NULL, /* class_finalize */
          NULL, /* class_data */
          sizeof (GHidMainMenu),
          0,    /* n_preallocs */
          (GInstanceInitFunc) ghid_main_menu_init,
        };

      mm_type = g_type_register_static (GTK_TYPE_MENU_BAR,
                                        "GHidMainMenu",
                                        &mm_info, 0);
    }

  return mm_type;
}

/*! \brief Create a new GHidMainMenu
 *
 *  \return a freshly-allocated GHidMainMenu
 */
GtkWidget *
ghid_main_menu_new (GCallback action_cb,
                    void (*special_key_cb) (const char *accel,
                                            GtkAction *action,
                                            const Resource *node))
{
  GHidMainMenu *mm = g_object_new (GHID_MAIN_MENU_TYPE, NULL);

  mm->accel_group = gtk_accel_group_new ();
  mm->action_group = gtk_action_group_new ("MainMenu");

  mm->layer_view_pos = 0;
  mm->layer_pick_pos = 0;
  mm->route_style_pos = 0;
  mm->n_layer_views = 0;
  mm->n_layer_picks = 0;
  mm->n_route_styles = 0;
  mm->layer_view_shell = NULL;
  mm->layer_pick_shell = NULL;
  mm->route_style_shell = NULL;

  mm->special_key_cb = special_key_cb;
  mm->action_cb = action_cb;
  mm->actions = NULL;
  mm->popup_table = g_hash_table_new (g_str_hash, g_str_equal);

  return GTK_WIDGET (mm);
}

/*! \brief Turn a pcb resource into the main menu */
void
ghid_main_menu_add_resource (GHidMainMenu *menu, const Resource *res)
{
  ghid_main_menu_real_add_resource (menu, GTK_MENU_SHELL (menu), res, "");
}

/*! \brief Turn a pcb resource into a popup menu */
void
ghid_main_menu_add_popup_resource (GHidMainMenu *menu, const char *name,
                                   const Resource *res)
{
  GtkWidget *new_menu = gtk_menu_new ();
  g_object_ref_sink (new_menu);
  ghid_main_menu_real_add_resource (menu, GTK_MENU_SHELL (new_menu), res, "");
  g_hash_table_insert (menu->popup_table, (gpointer) name, new_menu);
  gtk_widget_show_all (new_menu);
}

/*! \brief Returns a registered popup menu by name */
GtkMenu *
ghid_main_menu_get_popup (GHidMainMenu *menu, const char *name)
{
  return g_hash_table_lookup (menu->popup_table, name);
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
ghid_main_menu_update_toggle_state (GHidMainMenu *menu,
                                    void (*cb) (GtkAction *,
                                                const char *toggle_flag,
                                                const char *active_flag))
{
  GList *list;
  for (list = menu->actions; list; list = list->next)
    {
      Resource *res = g_object_get_data (G_OBJECT (list->data), "resource");
      const char *tf = g_object_get_data (G_OBJECT (list->data),
                                          "checked-flag");
      const char *af = g_object_get_data (G_OBJECT (list->data),
                                          "active-flag");
      g_signal_handlers_block_by_func (G_OBJECT (list->data),
                                       menu->action_cb, res);
      cb (GTK_ACTION (list->data), tf, af);
      g_signal_handlers_unblock_by_func (G_OBJECT (list->data),
                                         menu->action_cb, res);
    }
}

/*! \brief Installs or updates layer selector items */
void
ghid_main_menu_install_layer_selector (GHidMainMenu *mm,
                                       GHidLayerSelector *ls)
{
  GList *children, *iter;

  /* @layerview */
  if (mm->layer_view_shell)
    {
      /* Remove old children */
      children = gtk_container_get_children
                   (GTK_CONTAINER (mm->layer_view_shell));
      for (iter = g_list_nth (children, mm->layer_view_pos);
           iter != NULL && mm->n_layer_views > 0;
           iter = g_list_next (iter), mm->n_layer_views --)
        gtk_container_remove (GTK_CONTAINER (mm->layer_view_shell),
                              iter->data);
      g_list_free (children);

      /* Install new ones */
      mm->n_layer_views = ghid_layer_selector_install_view_items
                            (ls, mm->layer_view_shell, mm->layer_view_pos);
    }

  /* @layerpick */
  if (mm->layer_pick_shell)
    {
      /* Remove old children */
      children = gtk_container_get_children
                   (GTK_CONTAINER (mm->layer_pick_shell));
      for (iter = g_list_nth (children, mm->layer_pick_pos);
           iter != NULL && mm->n_layer_picks > 0;
           iter = g_list_next (iter), mm->n_layer_picks --)
        gtk_container_remove (GTK_CONTAINER (mm->layer_pick_shell),
                              iter->data);
      g_list_free (children);

      /* Install new ones */
      mm->n_layer_picks = ghid_layer_selector_install_pick_items
                            (ls, mm->layer_pick_shell, mm->layer_pick_pos);
    }
}

/*! \brief Installs or updates route style selector items */
void
ghid_main_menu_install_route_style_selector (GHidMainMenu *mm,
                                             GHidRouteStyleSelector *rss)
{
  GList *children, *iter;
  /* @routestyles */
  if (mm->route_style_shell)
    {
      /* Remove old children */
      children = gtk_container_get_children
                   (GTK_CONTAINER (mm->route_style_shell));
      for (iter = g_list_nth (children, mm->route_style_pos);
           iter != NULL && mm->n_route_styles > 0;
           iter = g_list_next (iter), mm->n_route_styles --)
        gtk_container_remove (GTK_CONTAINER (mm->route_style_shell),
                              iter->data);
      g_list_free (children);
      /* Install new ones */
      mm->n_route_styles = ghid_route_style_selector_install_items
                             (rss, mm->route_style_shell, mm->route_style_pos);
    }
}

/*! \brief Returns the menu bar's accelerator group */
GtkAccelGroup *
ghid_main_menu_get_accel_group (GHidMainMenu *menu)
{
  if (menu == NULL) {
    Message("ghid: can't initialize the menu - is your gpcb-menu.res valid?\n");
    exit(1);
  }
  return menu->accel_group;
}


void d1() {}

void ghid_create_menu(const char *menu[], const char *action, const char *mnemonic, const char *accel, const char *tip)
{
	char *path, *path_end;
	int n, plen;

	plen = 1; /* for the \0 */
	for(n = 0; menu[n] != NULL; n++)
		plen += strlen(menu[n])+1; /* +1 for the leading slash */

	path = path_end = malloc(plen);
	*path = '\0';

	for(n = 0; menu[n] != NULL; n++) {
		int last  = (menu[n+1] == NULL);
		int first = (n == 0);
		GtkWidget *w;
		Resource *res;

		/* check if the current node exists */
		*path_end = '/';
		path_end++;
		strcpy(path_end, menu[n]);
		if (g_hash_table_lookup(menu_hash, path) != NULL) {
			path_end += strlen(menu[n]);
			continue;
		}

		/* if not, revert path to the parent's path */
		path_end--;
		*path_end = '\0';

		/* look up the parent */
		if (first)
			w = ghidgui->menu_bar;
		else
			w = g_hash_table_lookup(menu_hash, path);

		if (!last) {
			int flags = first ? (FLAG_S | FLAG_NV | FLAG_V) /* 7 */ : (FLAG_V | FLAG_S) /* 5 */;
			res = resource_create_menu(menu[n], NULL, NULL, NULL, NULL, flags);
		}
		else
			res = resource_create_menu(menu[n], action, mnemonic, accel, tip, FLAG_NS | FLAG_NV | FLAG_V);

/*
	fprintf(stderr, "l1\n");
	print_menu_shell((GTK_MENU_SHELL(w)));
	fprintf(stderr, "Add: '%s' (%p) -> '%s'\n", path, w, menu[n]);
*/

		ghid_main_menu_real_add_resource (GHID_MAIN_MENU(ghidgui->menu_bar), GTK_MENU_SHELL(w), res, path);

/*
	fprintf(stderr, "l2\n");
	print_menu_shell((GTK_MENU_SHELL(w)));
*/

		*path_end = '/';
		path_end += strlen(menu[n])+1;
	}

/* make sure new menu items appear on screen */
  gtk_widget_show_all (ghidgui->menu_bar);

	free(path);
}
