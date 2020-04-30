order_ctx_t order_ctx;

static void order_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	order_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(order_ctx_t));
}

static int order_dialog(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pcb_order_imp_t *imp;
	int n;

	if (order_ctx.active)
		return -1; /* do not open another */

	if (pcb_order_imps.used == 0) {
		rnd_message(RND_MSG_ERROR, "OrderPCB(): there are no ordering plugins compiled/loaded\n");
		return -1;
	}

	order_ctx.names.used = 0;
	vtp0_enlarge(&order_ctx.names, pcb_order_imps.used);
	for(n = 0; n < pcb_order_imps.used; n++) {
		imp = pcb_order_imps.array[n];
		vtp0_set(&order_ctx.names, n, (char *)imp->name);
	}
	vtp0_set(&order_ctx.names, n, NULL);

	RND_DAD_BEGIN_VBOX(order_ctx.dlg);
		RND_DAD_COMPFLAG(order_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABBED(order_ctx.dlg, order_ctx.names.array);
			RND_DAD_COMPFLAG(order_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_LEFT_TAB);
			for(n = 0; n < pcb_order_imps.used; n++) {
				imp = pcb_order_imps.array[n];
				if (imp->load_fields(pcb_order_imps.array[n], &order_ctx) == 0) {
					RND_DAD_BEGIN_VBOX(order_ctx.dlg);
						RND_DAD_COMPFLAG(order_ctx.dlg, RND_HATF_EXPFILL);

						imp->populate_dad(pcb_order_imps.array[n], &order_ctx);
					RND_DAD_END(order_ctx.dlg);
				}
				else
					RND_DAD_LABEL(order_ctx.dlg, "Failed to determine form fields");
			}

		RND_DAD_END(order_ctx.dlg);
		RND_DAD_BUTTON_CLOSES(order_ctx.dlg, clbtn);
	RND_DAD_END(order_ctx.dlg);

	/* set up the context */
	order_ctx.active = 1;

	RND_DAD_NEW("order", order_ctx.dlg, "Order PCB", &order_ctx, rnd_false, order_close_cb);
	return 0;
}

void pcb_order_dad_field(order_ctx_t *octx, pcb_order_field_t *f)
{
	RND_DAD_BEGIN_HBOX(octx->dlg);
		RND_DAD_LABEL(octx->dlg, f->name);
		RND_DAD_BEGIN_VBOX(octx->dlg);
			RND_DAD_COMPFLAG(octx->dlg, RND_HATF_EXPFILL);
		RND_DAD_END(octx->dlg);
		switch(f->type) {
			case RND_HATT_ENUM:
				RND_DAD_ENUM(octx->dlg, f->enum_vals);
				RND_DAD_DEFAULT_NUM(octx->dlg, f->val.lng);
				break;
			case RND_HATT_INTEGER:
				RND_DAD_INTEGER(octx->dlg, "");
				RND_DAD_DEFAULT_NUM(octx->dlg, f->val.lng);
				break;
			case RND_HATT_COORD:
				RND_DAD_COORD(octx->dlg, "");
				RND_DAD_DEFAULT_NUM(octx->dlg, f->val.crd);
				break;
			case RND_HATT_STRING:
				RND_DAD_STRING(octx->dlg);
				RND_DAD_DEFAULT_PTR(octx->dlg, f->val.str);
				break;
			case RND_HATT_LABEL: break;
			default:
				RND_DAD_LABEL(octx->dlg, "<invalid type>");
		}
		f->wid = RND_DAD_CURRENT(octx->dlg);
	RND_DAD_END(octx->dlg);
}
