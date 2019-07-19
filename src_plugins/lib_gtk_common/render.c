#include "config.h"
#include <gtk/gtk.h>
#include "gui.h"
#include "glue_common.h"
#include "wt_preview.h"


void pcb_gtk_previews_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom)
{
	pcb_box_t screen;
	screen.X1 = left; screen.X2 = right;
	screen.Y1 = top; screen.Y2 = bottom;
	pcb_gtk_preview_invalidate(&ghidgui->common, &screen);
}

void pcb_gtk_previews_invalidate_all(void)
{
	pcb_gtk_preview_invalidate(&ghidgui->common, NULL);
}

