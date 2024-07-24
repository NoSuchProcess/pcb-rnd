/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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


int pcb_compat_route_style_via_save(pcb_data_t *data, const pcb_route_style_t *rst, rnd_coord_t *drill_dia, rnd_coord_t *pad_dia, rnd_coord_t *mask)
{
	/* load defaults */
	*drill_dia = RND_MM_TO_COORD(0.42);
	*pad_dia = RND_MM_TO_COORD(4.2);
	*mask = 0;

	if (rst->via_proto_set) {
		pcb_pstk_compshape_t cshape;
		rnd_bool plated;
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto_(data, rst->via_proto);

		if ((proto != NULL) && (proto->tr.used > 0)) {
			if (pcb_pstk_export_compat_proto(proto, drill_dia, pad_dia, mask, &cshape, &plated)) {
				if ((*drill_dia <= 0) || !plated || (cshape != PCB_PSTK_COMPAT_ROUND))
					pcb_io_incompat_save(data, NULL, "route-style", "Route style's via padstack proto is too complex for old via description", "Use a simpler via style: copper shapes only, on all copper layers, all circle and of the same size, plus a plated round hole - gEDA/pcb can't handle anything more complex.");
				else if (*mask > 0)
					pcb_io_incompat_save(data, NULL, "route-style", "Route style's via padstack proto has a mask opening that is not saved", "Use tented vias only (no mask shape); pcb-rnd can load gEDA/pcb's 2018 file format that can specify a mask opening but pcb-rnd saves an older gEDA/pcb file format version that doesn't yet have via mask opening value in routing style");
			}
			else
				return pcb_io_incompat_save(data, NULL, "route-style", "Failed to convert route style's via padstack proto to old via description", "Use a simpler via style: copper shapes only, on all copper layers, all circle and of the same size, plus a plated round hole - gEDA/pcb can't handle anything more complex.");
		}
		else
			return pcb_io_incompat_save(data, NULL, "route-style", "Invalid route style via prototype (does not exist)", "old gEDA/PCB format requires a via geometry - exporting a dummy one");
	}
	else
		return pcb_io_incompat_save(data, NULL, "route-style", "Invalid route style: no via prototype", "old gEDA/PCB format requires a via geometry - exporting a dummy one");

	return 0;
}

int pcb_compat_route_style_via_load(pcb_data_t *data, pcb_route_style_t *rst, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t mask)
{
	rnd_cardinal_t pid = pcb_pstk_new_compat_via_proto(data, drill_dia, pad_dia, mask, PCB_PSTK_COMPAT_ROUND, 1, 0);

	if (pid == -1)
		return -1;

	rst->via_proto = pid;
	rst->via_proto_set = 1;

	return 0;
}
