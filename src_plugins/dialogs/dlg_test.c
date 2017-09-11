
static const char dlg_test_syntax[] = "dlg_test()\n";
static const char dlg_test_help[] = "test the attribute dialog";
static void pcb_act_attr_chg(void *hid_ctx, pcb_hid_attribute_t *attr);
static int attr_idx, attr_idx2;
static int pcb_act_dlg_test(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *vals[] = { "foo", "bar", "baz", NULL };

	PCB_DAD_DECL(foo);
	PCB_DAD_BEGIN_VBOX(foo);
		PCB_DAD_LABEL(foo, "text1");
		PCB_DAD_BEGIN_TABLE(foo, 3);
			PCB_DAD_LABEL(foo, "text2a");
			PCB_DAD_LABEL(foo, "text2b");
			PCB_DAD_LABEL(foo, "text2c");
			PCB_DAD_LABEL(foo, "text2d");
		PCB_DAD_END(foo);
		PCB_DAD_LABEL(foo, "text3");

		PCB_DAD_ENUM(foo, vals);
			PCB_DAD_CHANGE_CB(foo, pcb_act_attr_chg);
			attr_idx = PCB_DAD_CURRENT(foo);
		PCB_DAD_INTEGER(foo, "text2e");
			PCB_DAD_MINVAL(foo, 1);
			PCB_DAD_MAXVAL(foo, 10);
			PCB_DAD_DEFAULT(foo, 3);
			PCB_DAD_CHANGE_CB(foo, pcb_act_attr_chg);
			attr_idx2 = PCB_DAD_CURRENT(foo);
		PCB_DAD_BUTTON(foo, "update!");
			PCB_DAD_CHANGE_CB(foo, pcb_act_attr_chg);
	PCB_DAD_END(foo);

	PCB_DAD_RUN(foo, "dlg_test", "attribute dialog test");

	PCB_DAD_FREE(foo);
	return 0;
}

static void pcb_act_attr_chg(void *hid_ctx, pcb_hid_attribute_t *attr)
{
	static pcb_hid_attr_val_t val;
	static pcb_bool st;
	printf("Chg\n");

	st = !st;
	val.int_value = (val.int_value + 1) % 3;
/*	pcb_gui->att_dlg_widget_state(hid_ctx, attr_idx, st);*/

	pcb_gui->att_dlg_set_value(hid_ctx, attr_idx, &val);
}
