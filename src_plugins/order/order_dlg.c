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
		return -1; /* do not open another */

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

	PCB_DAD_BEGIN_VBOX(order_ctx.dlg);
		PCB_DAD_COMPFLAG(order_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABBED(order_ctx.dlg, order_ctx.names.array);
			PCB_DAD_COMPFLAG(order_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_LEFT_TAB);
			for(n = 0; n < pcb_order_imps.used; n++) {
				imp = pcb_order_imps.array[n];
				if (imp->load_fields(pcb_order_imps.array[n], &order_ctx) == 0) {
					PCB_DAD_BEGIN_VBOX(order_ctx.dlg);
						PCB_DAD_COMPFLAG(order_ctx.dlg, PCB_HATF_EXPFILL);

						imp->populate_dad(pcb_order_imps.array[n], &order_ctx);
					PCB_DAD_END(order_ctx.dlg);
				}
				else
					PCB_DAD_LABEL(order_ctx.dlg, "Failed to determine form fields");
			}

		PCB_DAD_END(order_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(order_ctx.dlg, clbtn);
	PCB_DAD_END(order_ctx.dlg);

	/* set up the context */
	order_ctx.active = 1;

	PCB_DAD_NEW("EDIT_THIS_ID", order_ctx.dlg, "EDIT THIS: title", &order_ctx, pcb_false, order_close_cb);
	return 0;
}

void pcb_order_dad_field(order_ctx_t *octx, pcb_order_field_t *f)
{
	PCB_DAD_BEGIN_HBOX(octx->dlg);
		PCB_DAD_LABEL(octx->dlg, f->name);
		PCB_DAD_BEGIN_VBOX(octx->dlg);
			PCB_DAD_COMPFLAG(octx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_END(octx->dlg);
		switch(f->type) {
			case PCB_HATT_ENUM:
				PCB_DAD_ENUM(octx->dlg, f->enum_vals);
				PCB_DAD_DEFAULT_NUM(octx->dlg, f->val.lng);
				break;
			case PCB_HATT_INTEGER:
				PCB_DAD_INTEGER(octx->dlg, "");
				PCB_DAD_DEFAULT_NUM(octx->dlg, f->val.lng);
				break;
			case PCB_HATT_COORD:
				PCB_DAD_COORD(octx->dlg, "");
				PCB_DAD_DEFAULT_NUM(octx->dlg, f->val.crd);
				break;
			case PCB_HATT_STRING:
				PCB_DAD_STRING(octx->dlg);
				PCB_DAD_DEFAULT_PTR(octx->dlg, f->val.str);
				break;
			case PCB_HATT_LABEL: break;
			default:
				PCB_DAD_LABEL(octx->dlg, "<invalid type>");
		}
	PCB_DAD_END(octx->dlg);
}
