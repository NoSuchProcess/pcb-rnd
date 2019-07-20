/* Coordinate conversions */
#include "compat_misc.h"
#include "hidlib_conf.h"

/* Px converts view->pcb, Vx converts pcb->view */
static inline int Vx(pcb_coord_t x)
{
	double rv;
	if (pcbhl_conf.editor.view.flip_x)
		rv = (ghidgui->port.view.ctx->hidlib->size_x - x - ghidgui->port.view.x0) / ghidgui->port.view.coord_per_px + 0.5;
	else
		rv = (x - ghidgui->port.view.x0) / ghidgui->port.view.coord_per_px + 0.5;
	return pcb_round(rv);
}

static inline int Vy(pcb_coord_t y)
{
	double rv;
	if (pcbhl_conf.editor.view.flip_y)
		rv = (ghidgui->port.view.ctx->hidlib->size_y - y - ghidgui->port.view.y0) / ghidgui->port.view.coord_per_px + 0.5;
	else
		rv = (y - ghidgui->port.view.y0) / ghidgui->port.view.coord_per_px + 0.5;
	return pcb_round(rv);
}

static inline int Vz(pcb_coord_t z)
{
	return pcb_round((double)z / ghidgui->port.view.coord_per_px + 0.5);
}

static inline double Vxd(pcb_coord_t x)
{
	double rv;
	if (pcbhl_conf.editor.view.flip_x)
		rv = (ghidgui->port.view.ctx->hidlib->size_x - x - ghidgui->port.view.x0) / ghidgui->port.view.coord_per_px;
	else
		rv = (x - ghidgui->port.view.x0) / ghidgui->port.view.coord_per_px;
	return rv;
}

static inline double Vyd(pcb_coord_t y)
{
	double rv;
	if (pcbhl_conf.editor.view.flip_y)
		rv = (ghidgui->port.view.ctx->hidlib->size_y - y - ghidgui->port.view.y0) / ghidgui->port.view.coord_per_px;
	else
		rv = (y - ghidgui->port.view.y0) / ghidgui->port.view.coord_per_px;
	return rv;
}

static inline double Vzd(pcb_coord_t z)
{
	return (double)z / ghidgui->port.view.coord_per_px;
}

static inline pcb_coord_t Px(int x)
{
	pcb_coord_t rv = x * ghidgui->port.view.coord_per_px + ghidgui->port.view.x0;
	if (pcbhl_conf.editor.view.flip_x)
		rv = ghidgui->port.view.ctx->hidlib->size_x - (x * ghidgui->port.view.coord_per_px + ghidgui->port.view.x0);
	return rv;
}

static inline pcb_coord_t Py(int y)
{
	pcb_coord_t rv = y * ghidgui->port.view.coord_per_px + ghidgui->port.view.y0;
	if (pcbhl_conf.editor.view.flip_y)
		rv = ghidgui->port.view.ctx->hidlib->size_y - (y * ghidgui->port.view.coord_per_px + ghidgui->port.view.y0);
	return rv;
}

static inline pcb_coord_t Pz(int z)
{
	return (z * ghidgui->port.view.coord_per_px);
}

/* Return non-zero if box would be rendered into a single dot */
static inline int pcb_gtk_1dot(pcb_coord_t penwidth, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double minw = ghidgui->port.view.coord_per_px;
	double dx = PCB_ABS(x1-x2) + penwidth, dy = PCB_ABS(y1-y2) + penwidth;
	return ((dx < minw) && (dy < minw));
}

/* Return non-zero if dot coords are within canvas extents */
static inline int pcb_gtk_dot_in_canvas(pcb_coord_t penwidth, double dx1, double dy1)
{
	penwidth/=2;
	return ((dx1+penwidth >= 0) && (dx1-penwidth <= ghidgui->port.view.canvas_width) && (dy1+penwidth >= 0) && (dy1-penwidth <= ghidgui->port.view.canvas_height));
}

