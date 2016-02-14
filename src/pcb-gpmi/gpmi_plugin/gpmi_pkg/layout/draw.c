#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/hid.h"
#include "src/error.h"

#define setup \
	hidGC gc = ctx->gc; \
	HID  *hid = ctx->hid; \
	if ((hid == NULL) && (gc == NULL)) Message("%s failed because of invalid hid or gc\n", __FUNCTION__); \
	if ((hid == NULL) && (gc == NULL)) return


void draw_set_color(dctx_t *ctx, const char *name)
{
	setup;
	hid->set_color(gc, name);
}

/*void set_line_cap(dctx_t *ctx, EndCapStyle style_);*/
void draw_set_line_width(dctx_t *ctx, int width)
{
	setup;
	hid->set_line_width(gc, width);
}

void draw_set_draw_xor(dctx_t *ctx, int xor)
{
	setup;
	hid->set_draw_xor(gc, xor);
}


void draw_set_draw_faded(dctx_t *ctx, int faded)
{
	setup;
	hid->set_draw_faded(gc, faded);
}

void draw_line(dctx_t *ctx, int x1, int y1, int x2, int y2)
{
	setup;
	hid->draw_line(gc, x1, y1, x2, y2);
}

