/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - inline functions used in multiple blocks
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

RND_INLINE void tedax_layer_fsave_layernet(FILE *f, pcb_netmap_t *nmap, pcb_any_obj_t *obj)
{
	long oid = 0;
	pcb_net_t *net = htpp_get(&nmap->o2n, obj);
	char constr[3], *end;
	const char *netname;

	end = constr;

	/* for now do not allow the autorouter to move or delete anything */
	*end++ = 'm';
	*end++ = 'd';

	if (end == constr)
		*end++ = '-';
	*end = '\0';

	/* calculate the netname */
	if (net != NULL) {
		netname = net->name;
		if (strncmp(netname, "netmap_anon_", 12) == 0)
			netname = "-";
	}
	else
		netname = "-";

	fprintf(f, " %ld %s %s\n", oid, netname, constr);
}
