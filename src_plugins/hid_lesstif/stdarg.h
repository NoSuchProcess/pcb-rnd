#include "xincludes.h"
#include "color.h"

extern Colormap lesstif_colormap;

extern Arg stdarg_args[];
extern int stdarg_n;
#define stdarg(t,v) XtSetArg(stdarg_args[stdarg_n], t, v), stdarg_n++

void stdarg_do_color(const pcb_color_t *value, char *which);
void stdarg_do_color_str(const char *value, char *which);

