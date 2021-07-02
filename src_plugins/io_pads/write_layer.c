typedef struct pads_lyt_map_s {
	int plid; /* pads side layer ID */
	const char *purpose;
	pcb_layer_type_t lyt;
} pads_lyt_map_t;

static const pads_lyt_map_t pads_lyt_map[] = {
	{21, NULL,       PCB_LYT_TOP | PCB_LYT_MASK},
	{22, NULL,       PCB_LYT_BOTTOM | PCB_LYT_PASTE},
	{23, NULL,       PCB_LYT_TOP | PCB_LYT_PASTE},
	/*{24, NULL,       PCB_LYT_FAB},*/
	{26, NULL,       PCB_LYT_TOP | PCB_LYT_SILK},
	{27, "assy",     PCB_LYT_TOP | PCB_LYT_DOC},
	{27, NULL,       PCB_LYT_BOUNDARY},
	{28, NULL,       PCB_LYT_BOTTOM | PCB_LYT_MASK},
	{29, NULL,       PCB_LYT_BOTTOM | PCB_LYT_SILK},
	{30, "assy",     PCB_LYT_BOTTOM | PCB_LYT_DOC},
	{0, NULL, 0}
};

#define FIRST_DOC_PLID 31

static int pads_max_plid(write_ctx_t *wctx)
{
	if ((wctx->ver >= 9.0) && (wctx->ver < 1000))
		return 250;
	return 30;
}

static void pads_map_layers(write_ctx_t *wctx)
{
	pcb_layergrp_t *g;
	rnd_layergrp_id_t gid;
	int copnext = 1, docnext = FIRST_DOC_PLID, maxplid = pads_max_plid(wctx);

	vti0_set(&wctx->gid2plid, 64, 0); /* enlarge to save on reallocs */
	vtp0_set(&wctx->plid2grp, wctx->pcb->LayerGroups.len, 0); /* enlarge to save on reallocs */
	for(gid = 0, g = wctx->pcb->LayerGroups.grp; gid < wctx->pcb->LayerGroups.len; gid++,g++) {
		if (g->ltype & PCB_LYT_SUBSTRATE)
			continue;
		if (g->ltype & PCB_LYT_COPPER) {
			if (copnext <= 20) {
				vti0_set(&wctx->gid2plid, gid, copnext);
				vtp0_set(&wctx->plid2grp, copnext, g);
				copnext++;
			}
			else {
				char *tmp = rnd_strdup_printf("Too many copper layers; not exporting layer group '%s'\n", g->name);
				pcb_io_incompat_save(wctx->pcb->Data, NULL, "layer-grp", tmp, "At the moment only 20 copper layers are supported on PADSC ASCII export. If you bump into this limitation, please contact pcb-rnd developers.");
				free(tmp);
			}
		}
		else { /* non-copper */
			const pads_lyt_map_t *m;

			for(m = pads_lyt_map; m->plid != 0; m++)
				if (((g->ltype & m->lyt) == m->lyt) && ((m->purpose == NULL) || (strcmp(m->purpose, g->purpose) == 0)))
					break;

			if (m->plid != 0) {
				vti0_set(&wctx->gid2plid, gid, m->plid);
				if (wctx->plid2grp.array[m->plid] != NULL) {
					pcb_layergrp_t *e = wctx->plid2grp.array[m->plid];
					char *tmp = rnd_strdup_printf("Multiple non-copper layer groups with the same type: '%s' and '%s'\n", e->name, g->name);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "layer-grp", tmp, "These layer groups will be merged in PADS ASCII");
					free(tmp);
				}
				else
					vtp0_set(&wctx->plid2grp, m->plid, g);
			}
			else { /* no match */
				if (docnext <= maxplid) {
					vti0_set(&wctx->gid2plid, gid, docnext);
					vtp0_set(&wctx->plid2grp, docnext, g);
					docnext++;
				}
				else {
					char *tmp = rnd_strdup_printf("Too many non-copper layers; not exporting layer group '%s'\n", g->name);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "layer-grp", tmp, "This version of PADS ASCII supports 30 layers only. Choose a newer version to have the 250 layer mode.");
					free(tmp);
				}
			}
		}
	}
}

static int pads_gid2plid(const write_ctx_t *wctx, const rnd_layergrp_id_t gid)
{
	if ((gid == -1) || (gid >= wctx->gid2plid.used))
		return -1;
	return wctx->gid2plid.array[gid];
}

static int pads_layer2plid(const write_ctx_t *wctx, pcb_layer_t *l)
{
	return pads_gid2plid(wctx, pcb_layer_get_group_(l));
}

/* returns 0 on error; doesn't handle copper yet */
static int pads_lyt2plid(const write_ctx_t *wctx, pcb_layer_type_t lyt, const char *purpose)
{
	const pads_lyt_map_t *m;
	for(m = pads_lyt_map; m->plid != 0; m++) {
		if (lyt == m->lyt) {
			if (purpose == NULL) {
				if (m->purpose == NULL)
					return m->plid;
				if (strcmp(purpose, m->purpose) == 0)
					return m->plid;
			}
			else {
				if (m->purpose == NULL)
					return m->plid;
			}
		}
	}

	return 0;
}


static int pads_write_blk_misc_layer_assoc(write_ctx_t *wctx, const pcb_layergrp_t *g)
{
	pcb_layer_type_t loc = g->ltype & PCB_LYT_ANYWHERE;
	rnd_layergrp_id_t agid;
	const struct {
		const char *assoc;
		const char *purpose;
		pcb_layer_type_t lyt;
	} *a, tbl[] = {
		{"ASSOCIATED_SILK_SCREEN", NULL,   PCB_LYT_SILK},
		{"ASSOCIATED_PASTE_MASK",  NULL,   PCB_LYT_PASTE},
		{"ASSOCIATED_SOLDER_MASK", NULL,   PCB_LYT_MASK},
		{"ASSOCIATED_ASSEMBLY",    "assy", PCB_LYT_DOC},
		{NULL, NULL, 0}
	};

	for(a = tbl; a->assoc != NULL; a++) {
		if (pcb_layergrp_listp(wctx->pcb, a->lyt | loc, &agid, 1, -1, a->purpose) == 1) {
			pcb_layergrp_t *g = pcb_get_layergrp(wctx->pcb, agid);
			if (g != NULL)
				fprintf(wctx->f, "%s %s\r\n", a->assoc, g->name);
		}
	}

	return 0;
}

static int pads_write_blk_misc_layers(write_ctx_t *wctx)
{
	int plid, max_plid;

	fprintf(wctx->f, "*MISC*      MISCELLANEOUS PARAMETERS\r\n\r\n");
	fprintf(wctx->f, "*REMARK*    PARENT_KEYWORD PARENT_VALUE\r\n");
	fprintf(wctx->f, "*REMARK*  [ {\r\n");
	fprintf(wctx->f, "*REMARK*       CHILD_KEYWORD CHILD_VALUE\r\n");
	fprintf(wctx->f, "*REMARK*     [ CHILD_KEYWORD CHILD_VALUE\r\n");
	fprintf(wctx->f, "*REMARK*     [ {\r\n");
	fprintf(wctx->f, "*REMARK*          GRAND_CHILD_KEYWORD GRAND_CHILD_VALUE [...]\r\n");
	fprintf(wctx->f, "*REMARK*       } ]]\r\n");
	fprintf(wctx->f, "*REMARK*    } ]\r\n\r\n");
	fprintf(wctx->f, "LAYER DATA\r\n");
	fprintf(wctx->f, "{\r\n");
	fprintf(wctx->f, "LAYER 0\r\n");
	fprintf(wctx->f, "{\r\n");
	fprintf(wctx->f, "LAYER_THICKNESS 0\r\n");
	fprintf(wctx->f, "DIELECTRIC 3.300000\r\n");
	fprintf(wctx->f, "}\r\n");

	max_plid = pads_max_plid(wctx);

	for(plid = 1; plid <= max_plid; plid++) {
		pcb_layergrp_t *g = NULL;
		
		if (plid < wctx->plid2grp.used)
			g = wctx->plid2grp.array[plid];

		fprintf(wctx->f, "LAYER %d\r\n", plid);
		fprintf(wctx->f, "{\r\n");

		if (g != NULL) {
			fprintf(wctx->f, "LAYER_NAME %s\r\n", g->name);

			if (g->ltype & PCB_LYT_COPPER) fprintf(wctx->f, "LAYER_TYPE ROUTING\r\n");
			else if (g->ltype & PCB_LYT_SILK) fprintf(wctx->f, "LAYER_TYPE SILK_SCREEN\r\n");
			else if (g->ltype & PCB_LYT_PASTE) fprintf(wctx->f, "LAYER_TYPE PASTE_MASK\r\n");
			else if (g->ltype & PCB_LYT_MASK) fprintf(wctx->f, "LAYER_TYPE SOLDER_MASK\r\n");
			else goto unassigned;

			if ((g->ltype & PCB_LYT_COPPER) && ((g->ltype & PCB_LYT_TOP) || (g->ltype & PCB_LYT_BOTTOM)))
				pads_write_blk_misc_layer_assoc(wctx, g);
		}
		else {
			fprintf(wctx->f, "LAYER_NAME Layer_%d\r\n", plid);
			unassigned:;
			fprintf(wctx->f, "LAYER_TYPE UNASSIGNED\r\n");
		}

		fprintf(wctx->f, "PLANE NONE\r\n");
		fprintf(wctx->f, "ROUTING_DIRECTION NO_PREFERENCE\r\n");
		fprintf(wctx->f, "VISIBLE Y\r\n");
		fprintf(wctx->f, "SELECTABLE Y\r\n");
		fprintf(wctx->f, "ENABLED Y\r\n");
		fprintf(wctx->f, "LAYER_THICKNESS 0\r\n");
		fprintf(wctx->f, "COPPER_THICKNESS 0\r\n");
		fprintf(wctx->f, "DIELECTRIC 0.000000\r\n");
		fprintf(wctx->f, "COST 0\r\n");
		fprintf(wctx->f, "}\r\n");
	}

	fprintf(wctx->f, "}\r\n\r\n");
	return 0;
}

static void pads_free_layers(write_ctx_t *wctx)
{
	vti0_uninit(&wctx->gid2plid);
	vtp0_uninit(&wctx->plid2grp);
}

