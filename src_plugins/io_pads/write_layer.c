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
	{28, NULL,       PCB_LYT_BOTTOM | PCB_LYT_MASK},
	{29, NULL,       PCB_LYT_BOTTOM | PCB_LYT_SILK},
	{30, "assy",     PCB_LYT_BOTTOM | PCB_LYT_DOC},
	{0, NULL, 0}
};

#define FIRST_DOC_PLID 31

static void pads_map_layers(write_ctx_t *wctx)
{
	pcb_layergrp_t *g;
	rnd_layergrp_id_t gid;
	int copnext = 1, docnext = FIRST_DOC_PLID;

	vti0_set(&wctx->gid2plid, 64, 0); /* enlarge to save on reallocs */
	vtp0_set(&wctx->plid2grp, wctx->pcb->LayerGroups.len, 0); /* enlarge to save on reallocs */
	for(gid = 0, g = wctx->pcb->LayerGroups.grp; gid < wctx->pcb->LayerGroups.len; gid++,g++) {
		if (g->ltype & PCB_LYT_SUBSTRATE)
			continue;
		if (g->ltype & PCB_LYT_COPPER) {
			vti0_set(&wctx->gid2plid, gid, copnext);
			vtp0_set(&wctx->plid2grp, copnext, g);
			copnext++;
		}
		else { /* non-copper */
			pads_lyt_map_t *m;

			for(m = pads_lyt_map; m->plid != NULL; m++)
				if (((g->ltype & m->lyt) == m->lyt) && ((m->purpose == NULL) || (strcmp(m->purpose, g->purpose) == 0)))
					break;

			if (m->plid != 0) {
				vti0_set(&wctx->gid2plid, gid, m->plid);
				if (wctx->plid2grp.array[m->plid] != NULL) {
					pcb_layergrp_t *e = wctx->plid2grp.array[m->plid];
					char *tmp = rnd_strdup_printf("Multiple non-copper layer groups with the same type: '%s' and '%s'\n", e->name, g->name);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "layer-grp", tmp, "These layer groups will be merged in PADS ASCII");
				}
				else
					vtp0_set(&wctx->plid2grp, m->plid, g);
			}
			else { /* no match */
				vti0_set(&wctx->gid2plid, gid, docnext);
				vtp0_set(&wctx->plid2grp, docnext, g);
				docnext++;
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

static void pads_free_layers(write_ctx_t *wctx)
{
	vti0_uninit(&wctx->gid2plid);
	vtp0_uninit(&wctx->plid2grp);
}

