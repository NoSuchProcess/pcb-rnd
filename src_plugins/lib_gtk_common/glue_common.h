#include "hid.h"
#include "gui.h"

void ghid_glue_common_init(const char *cookie);
void ghid_glue_common_uninit(const char *cookie);

void ghid_draw_area_update(GHidPort *out, GdkRectangle *rect);
