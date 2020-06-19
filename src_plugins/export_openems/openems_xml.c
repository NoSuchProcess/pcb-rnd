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

TODO("remove this once the function is moved and published in core")
extern const char *pcb_layergrp_thickness_attr(pcb_layergrp_t *grp, const char *namespace);

static void openems_wr_xml_layergrp_end(wctx_t *ctx)
{
	if (ctx->cond_sheet_open) {
		fprintf(ctx->f, "        </Primitives>\n");
		fprintf(ctx->f, "      </ConductingSheet>\n");
		ctx->cond_sheet_open = 0;
	}
}

static rnd_coord_t get_grp_elev(wctx_t *ctx, pcb_layergrp_t *g)
{
	rnd_layergrp_id_t from, to;

	if (pcb_layergrp_list(ctx->pcb, PCB_LYT_BOTTOM|PCB_LYT_COPPER, &from, 1) != 1) {
		ctx->elevation = 0;
		rnd_message(RND_MSG_ERROR, "Missing bottom copper layer group - can not simulate\n");
		return -1;
	}
	to = g - ctx->pcb->LayerGroups.grp;
	if (from == to)
		return 0;
	return pcb_stack_thickness(ctx->pcb, "openems", PCB_BRDTHICK_PRINT_ERROR, from, 1, to, 0, PCB_LYT_SUBSTRATE|PCB_LYT_COPPER);
}

static int openems_wr_xml_layergrp_begin(wctx_t *ctx, pcb_layergrp_t *g)
{
	rnd_coord_t th;
	pcb_layer_t *ly = NULL;

	openems_wr_xml_layergrp_end(ctx);

	if (g->len < 0) {
		/* shouldn't happen: we are not called for empty layer groups */
		return;
	}
	ly = pcb_get_layer(ctx->pcb->Data, g->lid[0]);

	fprintf(ctx->f, "      <ConductingSheet Name='%s' Conductivity='", g->name);
		print_lparm(ctx, g, "conductivity", HA_def_copper_cond, -1, NULL, NULL);
		rnd_fprintf(ctx->f, "' Thickness='%.09mm'>\n", th);

	if (ly != NULL) {
		fprintf(ctx->f, "        <FillColor R='%d' G='%d' B='%d' a='128'/>\n", ly->meta.real.color.r, ly->meta.real.color.g, ly->meta.real.color.b);
		fprintf(ctx->f, "        <EdgeColor R='%d' G='%d' B='%d' a='192'/>\n", ly->meta.real.color.r, ly->meta.real.color.g, ly->meta.real.color.b);
	}

	fprintf(ctx->f, "        <Primitives>\n");
	ctx->cond_sheet_open = 1;
	th = get_grp_elev(ctx, g);
	if (th < 0)
		return -1;
	ctx->elevation = RND_COORD_TO_MM(th);
	return 0;
}

static int openems_wr_xml_outline(wctx_t *ctx, pcb_layergrp_t *g)
{
	int n;
	pcb_any_obj_t *out1;
	const char *s;
	rnd_coord_t th = 0;

	s = pcb_layergrp_thickness_attr(g, "openems");
	if (s != NULL)
		th = rnd_get_value(s, NULL, NULL, NULL);

	if (th <= 0) {
		rnd_message(RND_MSG_ERROR, "Substrate thickness is missing or invalid - can't export\n");
		return -1;
	}

TODO("layer: consider multiple outline layers instead")
	out1 = pcb_topoly_find_1st_outline(ctx->pcb);

	rnd_fprintf(ctx->f, "          <LinPoly Priority='%d' Elevation='%f' Length='%mm' NormDir='2' CoordSystem='0'>\n", PRIO_SUBSTRATE, ctx->elevation, th);
	if (out1 != NULL) {
		long n;
		pcb_poly_t *p = pcb_topoly_conn(ctx->pcb, out1, PCB_TOPOLY_KEEP_ORIG | PCB_TOPOLY_FLOATING);

		for(n = 0; n < p->PointN; n++)
			rnd_fprintf(ctx->f, "            <Vertex X1='%mm' X2='%mm'/>\n", p->Points[n].X, -p->Points[n].Y);
		pcb_poly_free(p);
	}
	else {
		/* rectangular board size */
		rnd_fprintf(ctx->f, "            <Vertex X1='%mm' X2='%mm'/>\n", 0, 0);
		rnd_fprintf(ctx->f, "            <Vertex X1='%mm' X2='%mm'/>\n", ctx->pcb->hidlib.size_x, 0);
		rnd_fprintf(ctx->f, "            <Vertex X1='%mm' X2='%mm'/>\n", ctx->pcb->hidlib.size_x, -ctx->pcb->hidlib.size_y);
		rnd_fprintf(ctx->f, "            <Vertex X1='%mm' X2='%mm'/>\n", 0, -ctx->pcb->hidlib.size_y);
	}
	rnd_fprintf(ctx->f, "          </LinPoly>\n");
	return 0;
}


static int openems_wr_xml_grp_substrate(wctx_t *ctx, pcb_layergrp_t *g)
{
	rnd_coord_t th = get_grp_elev(ctx, g);

	if (th < 0)
		return -1;
	ctx->elevation = RND_COORD_TO_MM(th);

	fprintf(ctx->f, "      <Material Name='%s'>\n", g->name);
	fprintf(ctx->f, "        <FillColor R='220' G='180' B='100' a='50'/>\n");
	fprintf(ctx->f, "        <EdgeColor R='220' G='180' B='100' a='100'/>\n");
	fprintf(ctx->f, "        <Property Epsilon='");
		print_lparm(ctx, g, "epsilon", -1, HA_def_subst_epsilon, NULL, NULL);
		fprintf(ctx->f, "' Kappa='");
		print_lparm(ctx, g, "kappa", -1, HA_def_subst_kappa, NULL, NULL);
		fprintf(ctx->f, "'/>\n");
	fprintf(ctx->f, "        <Primitives>\n");
	openems_wr_xml_outline(ctx, g);
	fprintf(ctx->f, "        </Primitives>\n");
	fprintf(ctx->f, "      </Material>\n");
	return 0;
}

static int openems_wr_xml_layers(wctx_t *ctx)
{
	rnd_hid_expose_ctx_t ectx = {0};
	rnd_cardinal_t gid;
	int err = 0;

	fprintf(ctx->f, "    <Properties>\n");

	ectx.view.X1 = 0;
	ectx.view.Y1 = 0;
	ectx.view.X2 = ctx->pcb->hidlib.size_x;
	ectx.view.Y2 = ctx->pcb->hidlib.size_y;

	rnd_expose_main(&openems_hid, &ectx, NULL);
	openems_wr_xml_layergrp_end(ctx);

	/* export substrate bricks */
	for(gid = 0; gid < ctx->pcb->LayerGroups.len; gid++) {
		pcb_layergrp_t *g = &ctx->pcb->LayerGroups.grp[gid];
		if (g->ltype & PCB_LYT_SUBSTRATE)
			err |= openems_wr_xml_grp_substrate(ctx, g);
	}


	fprintf(ctx->f, "    </Properties>\n");
	return err;
}

static void openems_wr_xml_mesh_lines(wctx_t *ctx, pcb_mesh_t *mesh, char axis, pcb_mesh_lines_t *l, rnd_coord_t scale)
{
	rnd_cardinal_t n, i = 0, len = vtc0_len(&l->result);
	rnd_coord_t delta, at;

	fprintf(ctx->f, "      <%cLines>", axis);

	if (mesh->pml > 0) {
		delta = (l->result.array[1] - l->result.array[0]) * scale;
		at = l->result.array[0] + delta * (mesh->pml+1);
		for(n = 0; n < mesh->pml; n++) {
			at -= delta;
			rnd_fprintf(ctx->f, "%s%mm", (i++ == 0 ? "" : ","), (rnd_coord_t)(at*scale));
		}
	}

	for(n = 0; n < len; n++)
		rnd_fprintf(ctx->f, "%s%mm", (i++ == 0 ? "" : ","), (rnd_coord_t)(l->result.array[n]*scale));

	if (mesh->pml > 0) {
		delta = (l->result.array[len-1] - l->result.array[len-2]) * scale;
		at = l->result.array[len-1];
		for(n = 0; n < mesh->pml; n++) {
			at += delta;
			rnd_fprintf(ctx->f, "%s%mm", (i++ == 0 ? "" : ","), (rnd_coord_t)(at*scale));
		}
	}

	fprintf(ctx->f, "</%cLines>\n", axis);
}


static void openems_wr_xml_grid(wctx_t *ctx, pcb_mesh_t *mesh)
{
	fprintf(ctx->f, "    <RectilinearGrid DeltaUnit='0.001' CoordSystem='0'>\n");
	openems_wr_xml_mesh_lines(ctx, mesh, 'Y', &mesh->line[PCB_MESH_HORIZONTAL], -1);
	openems_wr_xml_mesh_lines(ctx, mesh, 'X', &mesh->line[PCB_MESH_VERTICAL], 1);
	openems_wr_xml_mesh_lines(ctx, mesh, 'Z', &mesh->line[PCB_MESH_Z], 1);
	fprintf(ctx->f, "    </RectilinearGrid>\n");
}

static int openems_wr_xml(wctx_t *ctx)
{
	pcb_mesh_t *mesh = pcb_mesh_get(MESH_NAME);
	char *exc;
	int err = 0;

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
	fprintf(ctx->f, "  </FDTD>\n");

	fprintf(ctx->f, "  <ContinuousStructure CoordSystem='0'>\n");
	fprintf(ctx->f, "    <BackgroundMaterial Epsilon='%f' Mue='%f' Kappa='0' Sigma='0'/>\n", ctx->options[HA_void_epsilon].dbl, ctx->options[HA_void_mue].dbl);

	err |= openems_wr_xml_layers(ctx);

	openems_wr_testpoints(ctx, ctx->pcb->Data);

	openems_wr_xml_grid(ctx, mesh);
	fprintf(ctx->f, "  </ContinuousStructure>\n");


	fprintf(ctx->f, "</openEMS>\n");
	return err;
}

