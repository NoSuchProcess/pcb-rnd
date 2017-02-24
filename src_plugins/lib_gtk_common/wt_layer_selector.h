#ifndef GHID_LAYER_SELECTOR_H__
#define GHID_LAYER_SELECTOR_H__

#include <gtk/gtk.h>

#define GHID_LAYER_SELECTOR_TYPE            (pcb_gtk_layer_selector_get_type ())
#define GHID_LAYER_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_LAYER_SELECTOR_TYPE, pcb_gtk_layer_selector_t))
#define GHID_LAYER_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_LAYER_SELECTOR_TYPE, pcb_gtk_layer_selector_class_t))
#define IS_GHID_LAYER_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_LAYER_SELECTOR_TYPE))
#define IS_GHID_LAYER_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_LAYER_SELECTOR_TYPE))
typedef struct pcb_gtk_layer_selector_s pcb_gtk_layer_selector_t;
typedef struct pcb_gtk_layer_selector_class_s pcb_gtk_layer_selector_class_t;

GType pcb_gtk_layer_selector_get_type(void);

/** Creates and returns a new freshly-allocated pcb_gtk_layer_selector_t widget */
GtkWidget *pcb_gtk_layer_selector_new(void);

/** Adds a layer to a pcb_gtk_layer_selector_t.
    \par Function Description
    This function adds an entry to a pcb_gtk_layer_selector_t, which will
    appear in the layer-selection list as well as visibility and selection
    menus (assuming this is a selectable layer). For the first 20 layers,
    keyboard accelerators will be added for selection/visibility toggling.
  
    If the user_id passed already exists in the layer selector, that layer
    will have its data overwritten with the new stuff.
  
    \param [in] ls            The selector to be acted on
    \param [in] user_id       An ID used to identify the layer; will be passed to selection/visibility callbacks
    \param [in] name          The name of the layer; will be used on selector and menus
    \param [in] color_string  The color of the layer on selector
    \param [in] visibile      Whether the layer is visible
    \param [in] activatable   Whether the layer appears in menus and can be selected
 */
void pcb_gtk_layer_selector_add_layer(pcb_gtk_layer_selector_t * ls,
																			gint user_id,
																			const gchar * name, const gchar * color_string, gboolean visible, gboolean activatable);

/** Installs the "Current Layer" menu items for a layer selector
    \par Function Description
    Takes a menu shell and installs menu items for layer selection in
    the shell, at the given position.
  
    \param [in] ls      The selector to be acted on
    \param [in] shell   The menu to install the items in
    \param [in] pos     The position in the menu to install items
  
    \return the number of items installed
 */
gint pcb_gtk_layer_selector_install_pick_items(pcb_gtk_layer_selector_t * ls, GtkMenuShell * shell, gint pos);

/** Installs the "Shown Layers" menu items for a layer selector
    \par Function Description
    Takes a menu shell and installs menu items for layer selection in
    the shell, at the given position.
  
    \param [in] ls      The selector to be acted on
    \param [in] shell   The menu to install the items in
    \param [in] pos     The position in the menu to install items
  
    \return the number of items installed
 */
gint pcb_gtk_layer_selector_install_view_items(pcb_gtk_layer_selector_t * ls, GtkMenuShell * shell, gint pos);

/** Returns the GtkAccelGroup of a layer selector
    \par Function Description
  
    \param [in] ls            The selector to be acted on
  
    \return the accel group of the selector
 */
GtkAccelGroup *pcb_gtk_layer_selector_get_accel_group(pcb_gtk_layer_selector_t * ls);

/** Toggles a layer's visibility
    \par Function Description
    Toggles the layer indicated by user_id, emitting a layer-toggle signal.
  
    \param [in] ls       The selector to be acted on
    \param [in] user_id  The ID of the layer to be affected
 */
void pcb_gtk_layer_selector_toggle_layer(pcb_gtk_layer_selector_t * ls, gint user_id);

/** Selects a layer
    \par Function Description
    Select the layer indicated by user_id, emitting a layer-select signal.
  
    \param [in] ls       The selector to be acted on
    \param [in] user_id  The ID of the layer to be affected
 */
void pcb_gtk_layer_selector_select_layer(pcb_gtk_layer_selector_t * ls, gint user_id);

/** Selects the next visible layer
    \par Function Description
    Used to ensure hidden layers are not active; if the active layer is
    visible, this function is a noop. Otherwise, it will look for the
    next layer that IS visible, and select that. Failing that, it will
    return FALSE.
  
    \param [in] ls       The selector to be acted on
  
    \return TRUE on success, FALSE if all selectable layers are hidden
 */
gboolean pcb_gtk_layer_selector_select_next_visible(pcb_gtk_layer_selector_t * ls);

/** Sets the colors of all layers in a layer-selector
    \par Function Description
    Updates the colors of a layer selector via a callback mechanism:
    the user_id of each layer is passed to the callback function,
    which returns a color string to update the layer's color, or NULL
    to leave it alone.
  
    \param [in] ls       The selector to be acted on
    \param [in] callback Takes the user_id of the layer and returns a color string
 */
void pcb_gtk_layer_selector_update_colors(pcb_gtk_layer_selector_t * ls, const gchar * (*callback) (int user_id));

/** Deletes layers from a layer selector
    \par Function Description
    Deletes layers according to a callback function: a return value of TRUE
    means delete, FALSE means leave it alone. Do not try to delete all layers
    using this function; with nothing left to select, pcb will likely go into
    an infinite recursion between pcb_hid_action() and g_signal().
  
    Separators will be deleted if the layer AFTER them is deleted.
  
    \param [in] ls       The selector to be acted on
    \param [in] callback Takes the user_id of the layer and returns a boolean
 */
void pcb_gtk_layer_selector_delete_layers(pcb_gtk_layer_selector_t * ls, gboolean(*callback) (int user_id));

/** Sets the visibility toggle-state of all layers
    \par Function Description
    Shows layers according to a callback function: a return value of TRUE
    means show, FALSE means hide.
  
    \param [in] ls       The selector to be acted on
    \param [in] callback Takes the user_id of the layer and returns a boolean
 */
void pcb_gtk_layer_selector_show_layers(pcb_gtk_layer_selector_t * ls, gboolean(*callback) (int user_id));

#endif
