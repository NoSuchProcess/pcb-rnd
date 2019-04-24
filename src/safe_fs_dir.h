#include "compat_inc.h"
DIR *pcb_opendir(const char *name);
struct dirent *pcb_readdir(DIR *dir);
int pcb_closedir(DIR *dir);
