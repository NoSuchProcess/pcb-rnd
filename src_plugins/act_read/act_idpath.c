/* includeed from act_read.c */

static const char pcb_acts_IDPList[] =
	"IDPList(alloc)\n"
	"IDPList(free|clear|print|dup|length, list)\n"
	"IDPList(get|pop|remove, list, idx)\n"
	"IDPList(prepend|append|push, list, idpath)"
	;
static const char pcb_acth_IDPList[] = "Basic idpath list manipulation.";
static fgw_error_t pcb_act_IDPList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd_;
	pcb_idpath_list_t *list;
	pcb_idpath_t *idp;
	int cmd;
	long idx;

	PCB_ACT_CONVARG(1, FGW_STR, IDPList, cmd_ = argv[1].val.str);

	cmd = act_read_keywords_sphash(cmd_);
	if (cmd == act_read_keywords_alloc) {
		list = calloc(sizeof(pcb_idpath_list_t), 1);
		fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_IDPATH_LIST, FGW_PTR | FGW_STRUCT, list);
		return 0;
	}
	PCB_ACT_CONVARG(2, FGW_IDPATH_LIST, IDPList, list = fgw_idpath_list(&argv[2]));

	if (!fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH_LIST))
		return FGW_ERR_PTR_DOMAIN;

	switch(cmd) {
		case act_read_keywords_clear:
			pcb_idpath_list_clear(list);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_length:
			PCB_ACT_IRES(pcb_idpath_list_length(list));
printf("LEN: %ld\n", res->val.nat_long);
			return 0;

		case act_read_keywords_free:
			fgw_ptr_unreg(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH_LIST);
			pcb_idpath_list_clear(list);
			free(list);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_append:
		case act_read_keywords_push:
		case act_read_keywords_prepend:
			PCB_ACT_CONVARG(3, FGW_IDPATH, IDPList, idp = fgw_idpath(&argv[3]));
			if (!fgw_ptr_in_domain(&pcb_fgw, &argv[3], PCB_PTR_DOMAIN_IDPATH))
				return FGW_ERR_PTR_DOMAIN;
			if (cmd == act_read_keywords_append)
				pcb_idpath_list_append(list, idp);
			else /* prepend or push */
				pcb_idpath_list_insert(list, idp);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_remove:
			PCB_ACT_CONVARG(3, FGW_LONG, IDPList, idx = argv[3].val.nat_long);
			idp = pcb_idpath_list_nth(list, idx);
			if (idp == NULL) {
				PCB_ACT_IRES(-1);
				return 0;
			}
			pcb_idpath_list_remove(idp);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_get:
			PCB_ACT_CONVARG(3, FGW_LONG, IDPList, idx = argv[3].val.nat_long);
			idp = pcb_idpath_list_nth(list, idx);
			if (idp == NULL) {
				res->type = FGW_PTR;
				res->val.ptr_struct = NULL;
				return 0;
			}
			fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_IDPATH, FGW_PTR, idp);
			return 0;

		case act_read_keywords_pop:
			idp = pcb_idpath_list_first(list);
			if (idp == NULL) {
				res->type = FGW_PTR;
				res->val.ptr_struct = NULL;
				return 0;
			}
			fgw_ptr_reg(&pcb_fgw, res, PCB_PTR_DOMAIN_IDPATH, FGW_PTR, idp);
			pcb_idpath_list_remove(idp);
			return 0;

		case act_read_keywords_print:
			break;
	}

	return -1;
}

static const char pcb_acts_IDP[] = "IDP([print|free], idpath)\n";
static const char pcb_acth_IDP[] = "Basic idpath manipulation.";
static fgw_error_t pcb_act_IDP(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd;
	pcb_idpath_t *idp;

	PCB_ACT_CONVARG(1, FGW_STR, IDP, cmd = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_IDPATH, IDPList, idp = fgw_idpath(&argv[2]));
	if ((idp == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;


	switch(act_read_keywords_sphash(cmd)) {
		case act_read_keywords_free:
			pcb_idpath_list_remove(idp);
			fgw_ptr_unreg(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH);
			free(idp);
			PCB_ACT_IRES(0);
			return 0;

		case act_read_keywords_print:
			break;
	}

	return -1;
}
