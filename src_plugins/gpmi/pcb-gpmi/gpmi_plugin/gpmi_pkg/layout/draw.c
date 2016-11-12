#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/hid.h"
#include "src/error.h"

#define setup(func) \
	hidGC gc = ctx->gc; \
	pcb_hid_t  *hid = ctx->hid; \
	if ((hid == NULL) && (gc == NULL)) Message(PCB_MSG_DEFAULT, "%s failed because of invalid hid or gc\n", func); \
	if ((hid == NULL) && (gc == NULL)) return


void draw_set_color(dctx_t *ctx, const char *name)
{
	setup("draw_set_color");
	hid->set_color(gc, name);
}

/*void set_line_cap(dctx_t *ctx, pcb_cap_style_t style_);*/
void draw_set_line_width(dctx_t *ctx, int width)
{
	setup("draw_set_line_width");
	hid->set_line_width(gc, width);
}

void draw_set_draw_xor(dctx_t *ctx, int xor)
{
	setup("draw_set_draw_xor");
	hid->set_draw_xor(gc, xor);
}


void draw_set_draw_faded(dctx_t *ctx, int faded)
{
	setup("draw_set_draw_faded");
	hid->set_draw_faded(gc, faded);
}

void draw_line(dctx_t *ctx, int x1, int y1, int x2, int y2)
{
	setup("draw_line");
	hid->draw_line(gc, x1, y1, x2, y2);
}

