/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

static long def_num_timesteps = 1000000000;
static double def_end_crit = 1e-05;
static long def_f_max = 2100000000;

static void openems_wr_xml(wctx_t *ctx)
{
	pcb_mesh_t *mesh = pcb_mesh_get(MESH_NAME);
	char *exc;


	fprintf(ctx->f, "<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>\n");
	fprintf(ctx->f, "<openEMS>\n");
	fprintf(ctx->f, "  <FDTD NumberOfTimesteps='%ld' endCriteria='%f' f_max='%ld'>\n", def_num_timesteps, def_end_crit, def_f_max);

	exc = pcb_openems_excitation_get(ctx->pcb, 0);
	if (exc != NULL) {
		fprintf(ctx->f, "    <Excitation %s>\n", exc);
		fprintf(ctx->f, "    </Excitation>\n");
	}
	else
		fprintf(ctx->f, "<!-- ERROR: no excitation selected -->\n");
	free(exc);

	if ((mesh != NULL) && (mesh->bnd[0] != NULL)) {
		fprintf(ctx->f, "    <BoundaryCond xmin='%s' xmax='%s' ymin='%s' ymax='%s' zmin='%s' zmax='%s'>\n", mesh->bnd[0], mesh->bnd[1], mesh->bnd[2], mesh->bnd[3], mesh->bnd[4], mesh->bnd[5]);
		fprintf(ctx->f, "    </BoundaryCond>\n");
	}

	fprintf(ctx->f, "  </FDTD>\n");
	fprintf(ctx->f, "</openEMS>\n");
}

