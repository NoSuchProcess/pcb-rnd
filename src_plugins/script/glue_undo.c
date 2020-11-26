/* Live scripting needs to undo using the host app; to make this host app
   API independent, the action interface should be used */

static long script_undo_action(rnd_hidlib_t *hl, const char *cmd)
{
	fgw_arg_t res, argv[2];

	argv[1].type = FGW_STR;
	argv[1].val.cstr = cmd;
	if (rnd_actionv_bin(hl, "Undo", &res, 2, argv) != 0)
		return -1;

	if (fgw_arg_conv(&rnd_fgw, &res, FGW_LONG) != 0)
		return -1;

	return res.val.nat_long;
}

static long get_undo_serial(rnd_hidlib_t *hl)
{
	return script_undo_action(hl, "GetSerial");
}

static long get_num_undo(rnd_hidlib_t *hl)
{
	return script_undo_action(hl, "GetNum");
}

static void inc_undo_serial(rnd_hidlib_t *hl)
{
	script_undo_action(hl, "IncSerial");
}

static void undo_above(rnd_hidlib_t *hl, long ser)
{
	fgw_arg_t res, argv[3];

	argv[1].type = FGW_STR;
	argv[1].val.cstr = "Above";
	argv[2].type = FGW_LONG;
	argv[2].val.nat_long = ser;
	rnd_actionv_bin(hl, "Undo", &res, 3, argv);
	fgw_arg_free(&rnd_fgw, &res);
}
