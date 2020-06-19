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

static void openems_wr_xml_grp_copper(wctx_t *ctx, pcb_layergrp_t *g)
{
TODO("Fix hardwired constants");
	fprintf(ctx->f, "      <ConductingSheet Name='%s' Conductivity='56000000' Thickness='7e-05'>\n", g->name);
	fprintf(ctx->f, "        <Primitives>\n");
	fprintf(ctx->f, "        </Primitives>\n");
	fprintf(ctx->f, "      </ConductingSheet>\n");
}

static void openems_wr_xml_grp_substrate(wctx_t *ctx, pcb_layergrp_t *g)
{
TODO("Fix hardwired constants");
	fprintf(ctx->f, "      <Material Name='%s'>\n", g->name);
	fprintf(ctx->f, "        <Property Epsilon='4.8' Kappa=''>");
	fprintf(ctx->f, "        </Property>");
	fprintf(ctx->f, "      </Material>\n");
}

static void openems_wr_xml_layers(wctx_t *ctx)
{
	rnd_cardinal_t gid;
	fprintf(ctx->f, "    <Properties>\n");
	for(gid = 0; gid < ctx->pcb->LayerGroups.len; gid++) {
		pcb_layergrp_t *g = &ctx->pcb->LayerGroups.grp[gid];
		if (g->ltype & PCB_LYT_COPPER)
			openems_wr_xml_grp_copper(ctx, g);
		else if (g->ltype & PCB_LYT_SUBSTRATE)
			openems_wr_xml_grp_substrate(ctx, g);
	}
	fprintf(ctx->f, "    </Properties>\n");
}

static void openems_wr_xml_mesh_lines(wctx_t *ctx, pcb_mesh_t *mesh, char axis, pcb_mesh_lines_t *l)
{
	rnd_cardinal_t n;
TODO("AddPML seems to modify the grid");
	fprintf(ctx->f, "      <%cLines>", axis);
	for(n = 0; n < vtc0_len(&l->result); n++)
		rnd_fprintf(ctx->f, "%s%mm", (n == 0 ? "" : ","), l->result.array[n]);
	fprintf(ctx->f, "</%cLines>\n", axis);
}


static void openems_wr_xml_grid(wctx_t *ctx, pcb_mesh_t *mesh)
{
	fprintf(ctx->f, "    <RectilinearGrid DeltaUnit='0.001' CoordSystem='0'>\n");
	openems_wr_xml_mesh_lines(ctx, mesh, 'Y', &mesh->line[PCB_MESH_HORIZONTAL]);
	openems_wr_xml_mesh_lines(ctx, mesh, 'X', &mesh->line[PCB_MESH_VERTICAL]);
	openems_wr_xml_mesh_lines(ctx, mesh, 'Z', &mesh->line[PCB_MESH_Z]);
	fprintf(ctx->f, "    </RectilinearGrid>\n");
}

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
	else {
		fprintf(ctx->f, "<!-- ERROR: no excitation selected or invalid excitation -->\n");
		rnd_message(RND_MSG_ERROR, "openems xml: no excitation selected or invalid excitation\n");
	}
	free(exc);

	if ((mesh != NULL) && (mesh->bnd[0] != NULL)) {
		fprintf(ctx->f, "    <BoundaryCond xmin='%s' xmax='%s' ymin='%s' ymax='%s' zmin='%s' zmax='%s'>\n", mesh->bnd[0], mesh->bnd[1], mesh->bnd[2], mesh->bnd[3], mesh->bnd[4], mesh->bnd[5]);
		fprintf(ctx->f, "    </BoundaryCond>\n");
	}

	fprintf(ctx->f, "  <ContinuousStructure CoordSystem='0'>\n");
	fprintf(ctx->f, "    <BackgroundMaterial Epsilon='%f' Mue='%f' Kappa='0' Sigma='0'/>\n", ctx->options[HA_void_epsilon].dbl, ctx->options[HA_void_mue].dbl);

	openems_wr_xml_layers(ctx);
	openems_wr_xml_grid(ctx, mesh);
	fprintf(ctx->f, "  </ContinuousStructure>\n");


	fprintf(ctx->f, "  </FDTD>\n");
	fprintf(ctx->f, "</openEMS>\n");
}

