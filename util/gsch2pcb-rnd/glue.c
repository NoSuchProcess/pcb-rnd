/* gsch2pcb-rnd
 *  (C) 2015..2016, Tibor 'Igor2' Palinkas
 *
 *  This program is free software which I release under the GNU General Public
 *  License. You may redistribute and/or modify this program under the terms
 *  of that license as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.  Version 2 is in the
 *  COPYRIGHT file in the top level directory of this distribution.
 *
 *  To get a copy of the GNU General Puplic License, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "config.h"
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>
#include <librnd/core/hidlib_conf.h>
#include "gsch2pcb_rnd_conf.h"
#include <libfungw/fungw.h>

/* glue for pcb-rnd core */

void rnd_hidlib_crosshair_move_to(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot) { }

void *rnd_hidlib_crosshair_suspend(rnd_hidlib_t *hl) { return NULL; }
void rnd_hidlib_crosshair_restore(rnd_hidlib_t *hl, void *susp_data) {}
void rnd_draw_marks(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode) {}

static const char rnd_conf_internal_arr[] = { 0 };
const char *rnd_conf_internal = rnd_conf_internal_arr;

void rnd_expose_preview(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *e) {}

void rnd_hidlib_adjust_attached_objects(rnd_hidlib_t *hl) {}
void rnd_expose_main(rnd_hid_t * hid, const rnd_hid_expose_ctx_t *ctx, rnd_xform_t *xform_caller) {}
void rnd_draw_attached(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode) {}
