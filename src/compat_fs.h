char *GetWorkingDirectory(char *);

/* Portable mkdir() implentation.
 * Check whether mkdir() is mkdir or _mkdir, and whether it takes one
 * or two arguments.  WIN32 mkdir takes one argument and POSIX takes
 * two.
 */
int pcb_mkdir(const char *path, int mode);
#if HAVE_MKDIR
#if MKDIR_TAKES_ONE_ARG
				 /* MinGW32 */
#include <io.h>									/* mkdir under MinGW only takes one argument */
#define MKDIR(a, b) mkdir(a)
#else
#define MKDIR(a, b) mkdir(a, b)
#endif
#else
#if HAVE__MKDIR
				 /* plain Windows 32 */
#define MKDIR(a, b) _mkdir(a)
#endif
#endif

#ifndef MKDIR
#	error "Don't know how to create a directory on this system."
#endif

int pcb_spawnvp(char **argv);
char *tempfile_name_new(char *name);
int tempfile_unlink(char *name);
