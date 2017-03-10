#ifndef PCB_GTK_RENDER_H
#define PCB_GTK_RENDER_H

#include <gtk/gtk.h>
#include <glib.h>
#include "conf.h"
#include "pcb_bool.h"

#define LEAD_USER_WIDTH           0.2	/* millimeters */
#define LEAD_USER_PERIOD          (1000 / 5)	/* 5fps (in ms) */
#define LEAD_USER_VELOCITY        3.	/* millimeters per second */
#define LEAD_USER_ARC_COUNT       3
#define LEAD_USER_ARC_SEPARATION  3.	/* millimeters */
#define LEAD_USER_INITIAL_RADIUS  10.	/* millimetres */
#define LEAD_USER_COLOR_R         1.
#define LEAD_USER_COLOR_G         1.
#define LEAD_USER_COLOR_B         0.

typedef struct pcb_lead_user_s {
	guint timeout;
	GTimer *timer;
	pcb_bool lead_user;
	pcb_coord_t radius;
	pcb_coord_t x;
	pcb_coord_t y;
} pcb_lead_user_t;

/* the only entry points we should see */
void ghid_gdk_install(pcb_gtk_common_t *common, pcb_hid_t *hid);
void ghid_gl_install(pcb_gtk_common_t *common, pcb_hid_t *hid);

#endif