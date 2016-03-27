/* Glue between plug_footrpint and pcb-rnd; this is a separate file so a
   different glue can be used with gsch2pcb */
#include "config.h"
#include "global.h"
#include "data.h"

const char *fp_get_library_shell(void)
{
	return Settings.LibraryShell;
}

