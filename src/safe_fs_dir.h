#include "compat_inc.h"
DIR *pcb_opendir(pcb_hidlib_t *hidlib, const char *name);
struct dirent *pcb_readdir(DIR *dir);
int pcb_closedir(DIR *dir);
