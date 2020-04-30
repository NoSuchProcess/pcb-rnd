#include <stdlib.h>
#include <librnd/core/plugins.h>

#include "layer_compat.c"
#include "pstk_compat.c"
#include "pstk_help.c"
#include "subc_help.c"
#include "elem_rot.c"

int pplg_check_ver_lib_compat_help(int ver_needed) { return 0; }

void pplg_uninit_lib_compat_help(void)
{
}

int pplg_init_lib_compat_help(void)
{
	RND_API_CHK_VER;
	return 0;
}
