#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <librnd/core/plugins.h>

int pplg_check_ver_lib_portynet(int ver_needed) { return 0; }

void pplg_uninit_lib_portynet(void)
{
}

int pplg_init_lib_portynet(void)
{
	RND_API_CHK_VER;
	return 0;
}
