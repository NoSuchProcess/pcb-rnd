#ifndef PLUG_IO_H
#define PLUG_IO_H
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
	int dummy;
} rnd_hidlib_t;

typedef struct {
	int dummy;
} pcb_plug_io_t;

typedef enum {
	pcb_plug_iot_dummy
} pcb_plug_iot_t;

static inline long rnd_file_size(rnd_hidlib_t *hidlib, const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1;
	if (st.st_size > LONG_MAX)
		return -1;
	return st.st_size;
}

FILE *rnd_fopen(rnd_hidlib_t *hidlib, const char *fn, const char *mode);

#endif
