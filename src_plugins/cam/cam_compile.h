/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin - compile/decompile jobs
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_CAM_COMPILE_H
#define PCB_CAM_COMPILE_H

typedef enum {
	PCB_CAM_DESC,
	PCB_CAM_PLUGIN,
	PCB_CAM_WRITE,
	PCB_CAM_PREFIX
} pcb_cam_inst_t;

typedef struct {
	pcb_cam_inst_t inst;
	union {
		struct {
			char *arg;
		} desc;
		struct {
			pcb_hid_t *exporter;
			int argc;
			char **argv;
		} plugin;
		struct {
			char *arg;
		} write;
		struct {
			char *arg;
		} prefix;
	} op;
} pcb_cam_code_t;


#define GVT(x) vtcc_ ## x
#define GVT_ELEM_TYPE pcb_cam_code_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

typedef struct {
	char *prefix;            /* strdup'd file name prefix from the last prefix command */
	pcb_hid_t *exporter;

	char *args;              /* strdup'd argument string from the last plugin command - already split up */
	char **argv;             /* [0] and [1] are for --cam; the rest point into args */
	int argc;

	vtcc_t code;
	void *vars;

	gds_t tmp;
} cam_ctx_t;


#endif
