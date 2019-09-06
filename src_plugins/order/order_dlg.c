order_ctx_t order_ctx;

static void order_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	order_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(order_ctx_t));
}

static int order_dialog(void)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pcb_order_imp_t *imp;
	int n;

	if (order_ctx.active)
		return; /* do not open another */

	if (pcb_order_imps.used == 0) {
		pcb_message(PCB_MSG_ERROR, "OrderPCB(): there are no ordering plugins compiled/loaded\n");
		return -1;
	}

	order_ctx.names.used = 0;
	vtp0_enlarge(&order_ctx.names, pcb_order_imps.used);
	for(n = 0; n < pcb_order_imps.used; n++) {
		imp = pcb_order_imps.array[n];
		vtp0_set(&order_ctx.names, n, (char *)imp->name);
	}
	vtp0_set(&order_ctx.names, n, NULL);

	PCB_DAD_BEGIN_TABBED(order_ctx.dlg, order_ctx.names.array);
		PCB_DAD_COMPFLAG(order_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_LEFT_TAB);
		for(n = 0; n < pcb_order_imps.used; n++) {
			imp = pcb_order_imps.array[n];
			PCB_DAD_BEGIN_VBOX(order_ctx.dlg);
				imp->populate_dad(pcb_order_imps.array[n], &order_ctx);
			PCB_DAD_END(order_ctx.dlg);
		}
	PCB_DAD_END(order_ctx.dlg);

	/* set up the context */
	order_ctx.active = 1;

	PCB_DAD_NEW("EDIT_THIS_ID", order_ctx.dlg, "EDIT THIS: title", &order_ctx, pcb_false, order_close_cb);
}
