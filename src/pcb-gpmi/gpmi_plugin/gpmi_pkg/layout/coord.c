#include "layout.h"
#include "src/pcb-printf.h"

double mil2pcb_multiplier()
{
	return 10000.0*2.54;
}

double mm2pcb_multiplier()
{
	return 1000000.0;
}

const char *current_grid_unit()
{
	Unit *u = Settings.grid_unit;
	if (u == NULL)
		return "";
	return u->suffix;
}
