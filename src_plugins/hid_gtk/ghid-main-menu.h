#ifndef GHID_MAIN_MENU_H__
#define GHID_MAIN_MENU_H__

#include "../src_plugins/lib_gtk_common/wt_layer_selector.h"
#include "../src_plugins/lib_gtk_common/wt_route_style.h"

#include "hid_cfg.h"
#include "hid_cfg_input.h"

G_BEGIN_DECLS										/* keep c++ happy */
#define GHID_MAIN_MENU_TYPE            (ghid_main_menu_get_type ())
#define GHID_MAIN_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_MAIN_MENU_TYPE, GHidMainMenu))
#define GHID_MAIN_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_MAIN_MENU_TYPE, GHidMainMenuClass))
#define IS_GHID_MAIN_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_MAIN_MENU_TYPE))
#define IS_GHID_MAIN_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_MAIN_MENU_TYPE))
typedef struct _GHidMainMenu GHidMainMenu;
typedef struct _GHidMainMenuClass GHidMainMenuClass;

GType ghid_main_menu_get_type(void);
GtkWidget *ghid_main_menu_new(GCallback action_cb);
void ghid_main_menu_add_node(GHidMainMenu * menu, const lht_node_t * base);
GtkAccelGroup *ghid_main_menu_get_accel_group(GHidMainMenu * menu);
void ghid_main_menu_update_toggle_state(GHidMainMenu * menu,
																				void (*cb) (GtkAction *, const char *toggle_flag, const char *active_flag));

void ghid_main_menu_add_popup_node(GHidMainMenu * menu, lht_node_t * base);

void ghid_main_menu_install_layer_selector(GHidMainMenu * mm, pcb_gtk_layer_selector_t * ls);
void ghid_main_menu_install_route_style_selector(GHidMainMenu * mm, pcb_gtk_route_style_t * rss);

void ghid_create_menu(const char *menu_path, const char *action, const char *mnemonic, const char *accel, const char *tip,
											const char *cookie);
int ghid_remove_menu(const char *menu_path);

extern pcb_hid_cfg_t *ghid_cfg;

G_END_DECLS											/* keep c++ happy */
#endif
