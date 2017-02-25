#ifndef GHID_MAIN_MENU_H__
#define GHID_MAIN_MENU_H__

#include "wt_layer_selector.h"
#include "wt_route_style.h"

#include "conf.h"
#include "hid_cfg.h"
#include "hid_cfg_input.h"
#include "conf_hid.h"

G_BEGIN_DECLS										/* keep c++ happy */
#define GHID_MAIN_MENU_TYPE            (ghid_main_menu_get_type ())
#define GHID_MAIN_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_MAIN_MENU_TYPE, GHidMainMenu))
#define GHID_MAIN_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_MAIN_MENU_TYPE, GHidMainMenuClass))
#define IS_GHID_MAIN_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_MAIN_MENU_TYPE))
#define IS_GHID_MAIN_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_MAIN_MENU_TYPE))
typedef struct _GHidMainMenu GHidMainMenu;
typedef struct _GHidMainMenuClass GHidMainMenuClass;

typedef struct pcb_gtk_menu_ctx_s {
	GtkWidget *menu_bar;
	conf_hid_id_t ghid_menuconf_id;
	void (*confchg_checkbox)(conf_native_t *cfg);
} pcb_gtk_menu_ctx_t;

GType ghid_main_menu_get_type(void);
GtkWidget *ghid_main_menu_new(GCallback action_cb);
void ghid_main_menu_add_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu * menu, const lht_node_t * base);
GtkAccelGroup *ghid_main_menu_get_accel_group(GHidMainMenu * menu);
void ghid_main_menu_update_toggle_state(GHidMainMenu * menu,
																				void (*cb) (GtkAction *, const char *toggle_flag, const char *active_flag));

void ghid_main_menu_add_popup_node(pcb_gtk_menu_ctx_t *ctx, GHidMainMenu *menu, lht_node_t *base);

void ghid_main_menu_install_layer_selector(GHidMainMenu * mm, pcb_gtk_layer_selector_t * ls);
void ghid_main_menu_install_route_style_selector(GHidMainMenu * mm, pcb_gtk_route_style_t * rss);

int ghid_remove_menu_widget(void *ctx, lht_node_t *nd);
int ghid_create_menu_widget(void *ctx_, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *menu_item);

/* Temporary back references to hid_gtk: */
void ghid_confchg_checkbox(conf_native_t *cfg);

G_END_DECLS											/* keep c++ happy */
#endif
