#include <librnd/core/compat_inc.h>
DIR *rnd_opendir(rnd_hidlib_t *hidlib, const char *name);
struct dirent *rnd_readdir(DIR *dir);
int rnd_closedir(DIR *dir);
