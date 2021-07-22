#ifndef PLUG_IO_H
#define PLUG_IO_H
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
	int dummy;
} pcb_plug_io_t;

typedef enum {
	pcb_plug_iot_dummy
} pcb_plug_iot_t;

#endif
