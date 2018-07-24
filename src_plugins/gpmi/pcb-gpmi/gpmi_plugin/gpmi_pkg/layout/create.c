#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/board.h"
#include "src/undo.h"
#include "src/conf_core.h"
#include "src/layer.h"
#include "src/compat_misc.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"


typedef struct flag_tr_s {
	int flag; /* flag or thermal */
	int gpmi, pcb;
} flag_tr_t;

static flag_tr_t flags[] = {
#warning TODO: get these from conf
#if 0
	{1, FL_SHOWNUMBER,   PCB_SHOWNUMBERFLAG},
	{1, FL_LOCALREF,     PCB_LOCALREFFLAG},
	{1, FL_CHECKPLANS,   PCB_CHECKPLANESFLAG},
	{1, FL_SHOWDRC,      PCB_SHOWPCB_FLAG_DRC},
	{1, FL_RUBBERBAND,   PCB_RUBBERBANDFLAG},
	{1, FL_DESCRIPTION,  PCB_DESCRIPTIONFLAG},
	{1, FL_NAMEONPCB,    PCB_NAMEONPCBFLAG},
	{1, FL_AUTODRC,      PCB_AUTOPCB_FLAG_DRC},
	{1, FL_ALLDIRECTION, PCB_ALLDIRECTIONFLAG},
	{1, FL_SWAPSTARTDIR, PCB_SWAPSTARTDIRFLAG},
	{1, FL_UNIQUENAME,   PCB_UNIQUENAMEFLAG},
	{1, FL_CLEARNEW,     PCB_CLEARNEWFLAG},
	{1, FL_SNAPPIN,      PCB_SNAPPCB_FLAG_PIN},
	{1, FL_SHOWMASK,     PCB_SHOWMASKFLAG},
	{1, FL_THINDRAW,     PCB_THINDRAWFLAG},
	{1, FL_ORTHOMOVE,    PCB_ORTHOMOVEFLAG},
	{1, FL_LIVEROUTE,    PCB_LIVEROUTEFLAG},
	{1, FL_THINDRAWPOLY, PCB_THINDRAWPOLYFLAG},
	{1, FL_LOCKNAMES,    PCB_LOCKNAMESFLAG},
	{1, FL_ONLYNAMES,    PCB_ONLYNAMESFLAG},
	{1, FL_NEWFULLPOLY,  PCB_NEWPCB_FLAG_FULLPOLY},
	{1, FL_HIDENAMES,    PCB_HIDENAMESFLAG},
#endif

	{0, FL_THERMALSTYLE1, 1},
	{0, FL_THERMALSTYLE2, 2},
	{0, FL_THERMALSTYLE3, 3},
	{0, FL_THERMALSTYLE4, 4},
	{0, FL_THERMALSTYLE5, 5},
	{0, 0, 0}
};

static pcb_flag_t get_flags(int in)
{
	flag_tr_t *f;
	static pcb_flag_t out;

	out.f = 0;
	memset(out.t, 0, sizeof(out.t));
	for(f = flags; f->gpmi != 0; f++) {
		if (in & f->gpmi) {
			if (f->flag)
				out.f |= f->pcb;
			else
				memset(out.t, f->pcb, sizeof(out.t));
		}
	}
	return out;
}

static void *layout_create_line_(pcb_layer_t *layer, int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags)
{
	void *line;

	line = pcb_line_new(layer, x1, y1, x2, y2, thickness, clearance, get_flags(flags));
	if (line != NULL) {
		pcb_undo_add_obj_to_create(PCB_OBJ_LINE, layer, line, line);
		return line;
	}
	return NULL;
}

layout_object_t *layout_create_line(const char *search_id, layer_id_t layer_id, int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags)
{
	pcb_layer_t *layer = pcb_get_layer(PCB->Data, layer_id);
	void *res;
	if (layer == NULL)
		return 0;
	res = layout_create_line_(layer, x1, y1, x2, y2, thickness, clearance, flags);
	return search_persist_created(search_id, layer_id, res, OM_LINE);
}

static void *layout_create_via_(int x, int y, int thickness, int clearance, int mask, int hole, const char *name, multiple layout_flag_t flags)
{
	void *pin;

	pin = pcb_pstk_new_compat_via(PCB->Data, x, y, hole, thickness, clearance, mask, PCB_PSTK_COMPAT_ROUND, 1);

	if (pin != NULL) {
		pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, pin, pin, pin);
		return pin;
	}
	return NULL;
}

layout_object_t *layout_create_via(const char *search_id, int x, int y, int thickness, int clearance, int mask, int hole, const char *name, multiple layout_flag_t flags)
{
	void *res = layout_create_via_(x, y, thickness, clearance, mask, hole, name, flags);
	return search_persist_created(search_id, -1, res, OM_VIA);
}

static void *layout_create_arc_(pcb_layer_t *layer, int x, int y, int width, int height, int sa, int dir, int thickness, int clearance, multiple layout_flag_t flags)
{
	void *arc;
	arc = pcb_arc_new(layer, x, y, width, height, sa, dir, thickness, clearance, get_flags(flags), pcb_true);
	if (arc != NULL) {
		pcb_undo_add_obj_to_create(PCB_OBJ_ARC, layer, arc, arc);
		return 0;
	}
	return NULL;
}

layout_object_t *layout_create_arc(const char *search_id, layer_id_t layer_id, int x, int y, int width, int height, int sa, int dir, int thickness, int clearance, multiple layout_flag_t flags)
{
	layout_search_t *s;
	void *res;
	pcb_layer_t *layer = pcb_get_layer(PCB->Data, layer_id);

	if (layer == NULL)
		return NULL;

	res = layout_create_arc_(layer, x, y, width, height, sa, dir, thickness, clearance, flags);
	return search_persist_created(search_id, layer_id, res, OM_ARC);
}

layout_object_t *layout_create_text(const char *search_id, layer_id_t layer_id, int x, int y, int direction, int scale, const char *str, multiple layout_flag_t flags)
{
	layout_search_t *s;
	void *res;
	pcb_layer_t *layer = pcb_get_layer(PCB->Data, layer_id);

	if (layer == NULL)
		return NULL;

	res = pcb_text_new(layer, pcb_font(PCB, 0, 1), x, y, direction, scale, pcb_strdup(str), get_flags(flags));
	return search_persist_created(search_id, layer_id, res, OM_ARC);
}



