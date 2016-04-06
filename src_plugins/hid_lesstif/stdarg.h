#include "xincludes.h"

extern Colormap lesstif_colormap;

extern Arg stdarg_args[];
extern int stdarg_n;
#define stdarg(t,v) XtSetArg(stdarg_args[stdarg_n], t, v), stdarg_n++

void stdarg_do_color(const char *value, char *which);

