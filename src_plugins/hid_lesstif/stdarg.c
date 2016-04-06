#include "stdarg.h"
Arg stdarg_args[30];
int stdarg_n;

extern Colormap lesstif_colormap;
extern Display *lesstif_display;

void stdarg_do_color(const char *value, char *which)
{
	XColor color;
	if (XParseColor(lesstif_display, lesstif_colormap, value, &color))
		if (XAllocColor(lesstif_display, lesstif_colormap, &color)) {
			stdarg(which, color.pixel);
		}
}
