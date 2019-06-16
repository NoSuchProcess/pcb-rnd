#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "xpm/question.xpm"
#include "xpm/warning.xpm"

const char **pcp_dlg_xpm_by_name(const char *name)
{
	if (strcmp(name, "question") == 0) return question_xpm;
	if (strcmp(name, "warning") == 0) return warning_xpm;
	return NULL;
}
