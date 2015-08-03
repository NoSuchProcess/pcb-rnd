#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/hid.h"

static HID *ddh;

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
	flush_debug_draw();
}

void debug_draw_finish(void)
{
	need_ddh;
	finish_debug_draw();
}

void *debug_draw_gc(void)
{
	need_ddh(NULL);
	return ddh->make_gc();
}
