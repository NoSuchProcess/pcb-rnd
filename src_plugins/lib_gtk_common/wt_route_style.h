#ifndef GHID_ROUTE_STYLE_H__
#define GHID_ROUTE_STYLE_H__

#include <gtk/gtk.h>

#include "config.h"
#include "route_style.h"

G_BEGIN_DECLS										/* keep c++ happy */
#define GHID_ROUTE_STYLE_TYPE            (pcb_gtk_route_style_get_type ())
#define GHID_ROUTE_STYLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_ROUTE_STYLE_TYPE, pcb_gtk_route_style_t))
#define GHID_ROUTE_STYLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_ROUTE_STYLE_TYPE, pcb_gtk_route_style_class_t))
#define IS_GHID_ROUTE_STYLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_ROUTE_STYLE_TYPE))
#define IS_GHID_ROUTE_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_ROUTE_STYLE_TYPE))
//typedef struct _GHidRouteStyleSelector GHidRouteStyleSelector;
//typedef struct _GHidRouteStyleSelectorClass GHidRouteStyleSelectorClass;
typedef struct pcb_gtk_route_style_s pcb_gtk_route_style_t;
typedef struct pcb_gtk_route_style_class_s pcb_gtk_route_style_class_t;

GType pcb_gtk_route_style_get_type(void);

/** Creates and returns a new freshly-allocated pcb_gtk_route_style_t object */
GtkWidget *pcb_gtk_route_style_new(void);

/** Installs the "Route Style" menu items.
    Takes a menu shell and installs menu items for route style selection in
    the shell, at the given position. Note that we aren't really guaranteed
    the ordering of these items, since our internal data structure is a hash
    table. This shouldn't be a problem.

    \param [in] rss     The selector to be acted on
    \param [in] shell   The menu to install the items in
    \param [in] pos     The position in the menu to install items

    \return the number of items installed
 */
gint pcb_gtk_route_style_install_items(pcb_gtk_route_style_t * rss, GtkMenuShell * shell, gint pos);

/** Adds a route style to a pcb_gtk_route_style_t.
    Note that the route style object passed to this function will be
    updated directly.

    \param [in] rss     The selector to be acted on
    \param [in] data    PCB's route style object describing the style
 */
void pcb_gtk_route_style_add_route_style(pcb_gtk_route_style_t * rss, pcb_route_style_t * data);

/** Selects a route style and emits a select-style signal
    \param [in] rss  The selector to be acted on
    \param [in] rst  The style to select

    \return TRUE if a style was selected, FALSE otherwise
 */
gboolean pcb_gtk_route_style_select_style(pcb_gtk_route_style_t * rss, pcb_route_style_t * rst);

/** Builds and runs the "edit route styles" dialog */
void pcb_gtk_route_style_edit_dialog(pcb_gtk_route_style_t * rss);

/** Returns the GtkAccelGroup of a route style selector
    \param [in] rss            The selector to be acted on

    \return the accel group of the selector
 */
GtkAccelGroup *pcb_gtk_route_style_get_accel_group(pcb_gtk_route_style_t * rss);

/** Sets a pcb_gtk_route_style_t selection to given values
    \par Function Description
    Given the line thickness, via size and clearance values of a route
    style, this function selects a route style with the given values.
    If there is no such style registered with the selector, nothing
    will happen. This function does not emit any signals.

    \param [in] rss        The selector to be acted on
    \param [in] Thick      pcb_coord_t  to match selection to
    \param [in] Hole       pcb_coord_t  to match selection to
    \param [in] Diameter   pcb_coord_t  to match selection to
    \param [in] Clearance  pcb_coord_t  to match selection to
 */
void pcb_gtk_route_style_sync(pcb_gtk_route_style_t * rss, pcb_coord_t Thick, pcb_coord_t Hole, pcb_coord_t Diameter,
															pcb_coord_t Clearance);

/** Removes all styles from a route style selector */
void pcb_gtk_route_style_empty(pcb_gtk_route_style_t * rss);

G_END_DECLS											/* keep c++ happy */
#endif
