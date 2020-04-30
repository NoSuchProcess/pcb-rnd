/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <librnd/config.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <genvector/gds_char.h>

#include <librnd/core/unit.h>
#include <librnd/core/grid.h>
#include <librnd/core/conf.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/pcb_bool.h>
#include <librnd/core/pcb-printf.h>

rnd_coord_t rnd_grid_fit(rnd_coord_t x, rnd_coord_t grid_spacing, rnd_coord_t grid_offset)
{
	x -= grid_offset;
	x = grid_spacing * rnd_round((double) x / grid_spacing);
	x += grid_offset;
	return x;
}

pcb_bool_t rnd_grid_parse(rnd_grid_t *dst, const char *src)
{
	const char *nsep;
	char *sep3, *sep2, *sep, *tmp, *size, *ox = NULL, *oy = NULL, *unit = NULL;
	rnd_bool succ;

	nsep = strchr(src, ':');
	if (nsep != NULL)
		src = nsep+1;
	else
		dst->name = NULL;

	/* remember where size starts */
	while(isspace(*src)) src++;
	sep = size = tmp = rnd_strdup(src);

	/* find optional offs */
	sep2 = strchr(sep, '@');
	if (sep2 != NULL) {
		sep = sep2;
		*sep = '\0';
		sep++;
		ox = sep;
		sep3 = strchr(sep, ',');
		if (sep3 != NULL) {
			*sep3 = '\0';
			sep3++;
			oy = sep;
		}
	}

	/* find optional unit switch */
	sep2 = strchr(sep, '!');
	if (sep2 != NULL) {
		sep = sep2;
		*sep = '\0';
		sep++;
		unit = sep;
	}

	/* convert */
	dst->size = pcb_get_value(size, NULL, NULL, &succ);
	if ((!succ) || (dst->size < 0))
		goto error;

	if (ox != NULL) {
		dst->ox = pcb_get_value(ox, NULL, NULL, &succ);
		if (!succ)
			goto error;
	}
	else
		dst->ox = 0;

	if (oy != NULL) {
		dst->oy = pcb_get_value(oy, NULL, NULL, &succ);
		if (!succ)
			goto error;
	}
	else
		dst->oy = 0;

	if (unit != NULL) {
		dst->unit = get_unit_struct(unit);
		if (dst->unit == NULL)
			goto error;
	}
	else
		dst->unit = NULL;

	/* success */
	free(tmp);

	if (nsep != NULL)
		dst->name = rnd_strndup(src, nsep-src-1);
	else
		dst->name = NULL;
	return pcb_true;

	error:;
	free(tmp);
	return pcb_false;
}

pcb_bool_t rnd_grid_append_print(gds_t *dst, const rnd_grid_t *src)
{
	if (src->size <= 0)
		return pcb_false;
	if (src->name != NULL) {
		gds_append_str(dst, src->name);
		gds_append(dst, ':');
	}
	pcb_append_printf(dst, "%$.08mH", src->size);
	if ((src->ox != 0) || (src->oy != 0))
		pcb_append_printf(dst, "@%$.08mH,%$.08mH", src->ox, src->oy);
	if (src->unit != NULL) {
		gds_append(dst, '!');
		gds_append_str(dst, src->unit->suffix);
	}
	return pcb_true;
}

char *rnd_grid_print(const rnd_grid_t *src)
{
	gds_t tmp;
	gds_init(&tmp);
	if (!rnd_grid_append_print(&tmp, src)) {
		gds_uninit(&tmp);
		return NULL;
	}
	return tmp.array; /* do not uninit tmp */
}

void rnd_grid_set(rnd_hidlib_t *hidlib, const rnd_grid_t *src)
{
	rnd_hidlib_set_grid(hidlib, src->size, pcb_true, src->ox, src->oy);
	if (src->unit != NULL)
		rnd_hidlib_set_unit(hidlib, src->unit);
}

void rnd_grid_free(rnd_grid_t *dst)
{
	free(dst->name);
	dst->name = NULL;
}

pcb_bool_t rnd_grid_list_jump(rnd_hidlib_t *hidlib, int dst)
{
	const rnd_conf_listitem_t *li;
	rnd_grid_t g;
	int max = rnd_conflist_length((rnd_conflist_t *)&rnd_conf.editor.grids);

	if (dst < 0)
		dst = 0;
	if (dst >= max)
		dst = max - 1;
	if (dst < 0)
		return pcb_false;

	rnd_conf_setf(RND_CFR_DESIGN, "editor/grids_idx", -1, "%d", dst);

	li = rnd_conflist_nth((rnd_conflist_t *)&rnd_conf.editor.grids, dst);
	/* clamp */
	if (li == NULL)
		return pcb_false;

	if (!rnd_grid_parse(&g, li->payload))
		return pcb_false;
	rnd_grid_set(hidlib, &g);
	rnd_grid_free(&g);

	return pcb_true;
}

pcb_bool_t rnd_grid_list_step(rnd_hidlib_t *hidlib, int stp)
{
	int dst = rnd_conf.editor.grids_idx;
	if (dst < 0)
		dst = -dst-1;
	return rnd_grid_list_jump(hidlib, dst + stp);
}

void rnd_grid_inval(void)
{
	if (rnd_conf.editor.grids_idx > 0)
		rnd_conf_setf(RND_CFR_DESIGN, "editor/grids_idx", -1, "%d", -1 - rnd_conf.editor.grids_idx);
}

