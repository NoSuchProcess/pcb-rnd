#include "hid.h"
#include "gui.h"

#include "../src_plugins/lib_gtk_common/dlg_topwin.h"


void ghid_glue_common_init(const char *cookie);
void ghid_glue_common_uninit(const char *cookie);

void ghid_draw_area_update(pcb_gtk_port_t *out, GdkRectangle *rect);

/* make sure the context is set to draw the whole widget size, which might
   be slightly larger than the original request */
#define PCB_GTK_PREVIEW_TUNE_EXTENT(ctx, allocation) \
do { \
	pcb_coord_t nx1, ny1, nx2, ny2; \
	nx1 = Px(0); nx2 = Px(allocation.width); \
	ny1 = Py(0); ny2 = Py(allocation.height); \
	if (nx1 < nx2) { \
		ctx->view.X1 = nx1; \
		ctx->view.X2 = nx2; \
	} \
	else { \
		ctx->view.X1 = nx2; \
		ctx->view.X2 = nx1; \
	} \
	if (ny1 < ny2) { \
		ctx->view.Y1 = ny1; \
		ctx->view.Y2 = ny2; \
	} \
	else { \
		ctx->view.Y1 = ny2; \
		ctx->view.Y2 = ny1; \
	} \
} while(0)

/* Redraw all previews intersecting the specified screenbox (in case of lr) */
void pcb_gtk_previews_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom);
void pcb_gtk_previews_invalidate_all(void);

/*** Internal calls, hid implementations won't need these ***/
void pcb_gtk_tw_ranges_scale(pcb_gtk_topwin_t *tw);
void pcb_gtk_note_event_location(GdkEventButton *ev);

void pcb_gtk_interface_input_signals_connect(void);
void pcb_gtk_interface_input_signals_disconnect(void);
void pcb_gtk_interface_set_sensitive(gboolean sensitive);
void pcb_gtk_port_ranges_changed(void);
void pcb_gtk_mode_cursor_main(void);

