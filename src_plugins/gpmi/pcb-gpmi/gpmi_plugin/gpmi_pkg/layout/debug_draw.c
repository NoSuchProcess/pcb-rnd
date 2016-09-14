#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/hid.h"
#include "src/error.h"

static HID *ddh = NULL;

#define need_ddh if (ddh == NULL) return


int debug_draw_request(void)
{
	ddh = gui->request_debug_draw();
	if (ddh == NULL)
		return 0;
	return 1;
}

void debug_draw_flush(void)
{
	need_ddh;
	gui->flush_debug_draw();
}

void debug_draw_finish(dctx_t *ctx)
{
	need_ddh;
	ddh->destroy_gc(ctx->gc);
	gui->finish_debug_draw();
	free(ctx);
	ddh = NULL;
}

dctx_t *debug_draw_dctx(void)
{
	dctx_t *ctx;
	hidGC gc;
	need_ddh(NULL);
	gc = ddh->make_gc();
	if (gc == NULL) {
		Message(PCB_MSG_DEFAULT, "debug_draw_dctx(): failed to make a new gc on ddh %p\n", (void *)ddh);
		return NULL;
	}

	ctx = malloc(sizeof(dctx_t));
	ctx->hid = ddh;
	ctx->gc = gc;
}
