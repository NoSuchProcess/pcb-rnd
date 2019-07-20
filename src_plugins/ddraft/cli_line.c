static int line_parse(char *line, int argc, cli_node_t *argv, pcb_box_t *box, int verbose, int annot)
{
	int n = 0;

	if (argv[n].type == CLI_FROM)
		n++;
	else if (argv[n].type == CLI_TO) {
		pcb_message(PCB_MSG_ERROR, "Incremental line drawing is not yet supported\n");
		return -1;
	}
	n = cli_apply_coord(argv, n, argc, &box->X1, &box->Y1, annot);
	if (n < 0) {
		if (verbose)
			pcb_message(PCB_MSG_ERROR, "Invalid 'from' coord\n");
		return -1;
	}
	if (argv[n].type != CLI_TO) {
		if (verbose)
			pcb_message(PCB_MSG_ERROR, "Missing 'to'\n");
		return -1;
	}
	n++;
	box->X2 = box->X1;
	box->Y2 = box->Y1;
	n = cli_apply_coord(argv, n, argc, &box->X2, &box->Y2, annot);
	if (n < 0) {
		if (verbose)
			pcb_message(PCB_MSG_ERROR, "Invalid 'to' coord\n");
		return -1;
	}
	if (n != argc) {
		if (verbose)
			pcb_message(PCB_MSG_ERROR, "Excess tokens after the to coord\n");
		return -1;
	}
	return 0;
}

static int line_exec(char *line, int argc, cli_node_t *argv)
{
	int res;
	pcb_box_t box;

	pcb_trace("line e: '%s'\n", line);

	memset(&box, 0, sizeof(box));
	res = line_parse(line, argc, argv, &box, 1, 0);
	if (res == 0) {
		pcb_trace("line_exec: %mm;%mm -> %mm;%mm\n", box.X1, box.Y1, box.X2, box.Y2);
		pcb_line_new(CURRENT, box.X1, box.Y1, box.X2, box.Y2,
			conf_core.design.line_thickness, 2 * conf_core.design.clearance,
			pcb_flag_make(conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0));
	}
	pcb_ddraft_attached_reset();
	return res;
}

static int line_edit(char *line, int cursor, int argc, cli_node_t *argv)
{
	int res;
	pcb_box_t box;

	pcb_ddraft_attached_reset();

	if (pcb_tool_next_id != pcb_ddraft_tool)
		pcb_tool_select_by_id(&PCB->hidlib, pcb_ddraft_tool);

	pcb_trace("line e: '%s':%d\n", line, cursor);
	memset(&box, 0, sizeof(box));
	res = line_parse(line, argc, argv, &box, 0, 1);
	if (res == 0) {
		pcb_ddraft_attached.line_valid = 1;
		pcb_ddraft_attached.line.Point1.X = box.X1;
		pcb_ddraft_attached.line.Point1.Y = box.Y1;
		pcb_ddraft_attached.line.Point2.X = box.X2;
		pcb_ddraft_attached.line.Point2.Y = box.Y2;
		pcb_gui->invalidate_all(pcb_gui);
	}
	return res;
}

static int get_rel_coord(int argc, cli_node_t *argv, int argn, pcb_coord_t *ox, pcb_coord_t *oy)
{
	int nto, res;

	for(nto = 0; nto < argc; nto++)
		if (argv[nto].type == CLI_TO)
			break;

	*ox = *oy = 0;
	res = 0;
	if (argv[res].type == CLI_FROM)
		res++;
	if (argn < nto) {
		res = cli_apply_coord(argv, res, argn, ox, oy, 0);
	}
	else {
		res = cli_apply_coord(argv, res, nto, ox, oy, 0); /* 'to' may be relative to 'from', so eval 'from' first */
		res |= cli_apply_coord(argv, nto+1, argn, ox, oy, 0);
	}

	return res;
}

static int line_click(char *line, int cursor, int argc, cli_node_t *argv)
{
	int argn = cli_cursor_arg(argc, argv, cursor);
	int replace = 0, by, res;
	pcb_coord_t ox, oy;
	char buff[CLI_MAX_INS_LEN];

	pcb_trace("line c: '%s':%d (argn=%d)\n", line, cursor, argn);
	cli_print_args(argc, argv);

	if (argn < 0) {
		/* at the end */
		argn = argc - 1;
		if (argn < 0) {
			/* empty arg list */
			pcb_snprintf(buff, sizeof(buff), " from %.08$$mm,%.08$$mm", pcb_crosshair.X, pcb_crosshair.Y);
			printf("append: '%s'\n", buff);
			cursor = cli_str_insert(line, strlen(line), buff, 1);
			goto update;
		}
	}

	*buff = '\0';
	by = argn;


	retry:;

	switch(argv[by].type) {
		case CLI_COORD:
			if (by > 0) {
				by--;
				replace = 1;
				goto retry;
			}
			/* fall through: when clicked at the first coord */
		case CLI_ABSOLUTE:
		case CLI_FROM:
		case CLI_TO:
			pcb_trace("abs");
			pcb_snprintf(buff, sizeof(buff), "%.08$$mm,%.08$$mm", pcb_crosshair.X, pcb_crosshair.Y);
			goto maybe_replace_after;
			break;
		case CLI_RELATIVE:
			res = get_rel_coord(argc, argv, argn, &ox, &oy);
			if (res < 0) {
				pcb_message(PCB_MSG_ERROR, "Failed to interpret coords already entered\n");
				return 0;
			}
			pcb_trace("rel from %$mm,%$mm", ox, oy);
			pcb_snprintf(buff, sizeof(buff), "%.08$$mm,%.08$$mm", pcb_crosshair.X - ox, pcb_crosshair.Y - oy);
			maybe_replace_after:;
			if ((by+1 < argc) && (argv[by+1].type == CLI_COORD)) {
				argn = by+1;
				replace = 1;
			}
			break;
		case CLI_ANGLE:
			res = get_rel_coord(argc, argv, argn, &ox, &oy);
			if (res < 0) {
				pcb_message(PCB_MSG_ERROR, "Failed to interpret coords already entered\n");
				return 0;
			}
			replace=1;
			pcb_snprintf(buff, sizeof(buff), "<%f", atan2(pcb_crosshair.Y - oy, pcb_crosshair.X - ox) * PCB_RAD_TO_DEG);
			break;
		case CLI_DIST:
			res = get_rel_coord(argc, argv, argn, &ox, &oy);
			if (res < 0) {
				pcb_message(PCB_MSG_ERROR, "Failed to interpret coords already entered\n");
				return 0;
			}
			replace=1;
			pcb_snprintf(buff, sizeof(buff), "~%.08$$mm", pcb_distance(pcb_crosshair.X, pcb_crosshair.Y, ox, oy));
			break;
		case_object: ;
			{
				void *p1, *p2, *p3;
				pcb_any_obj_t *o;
				if (pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_LINE | PCB_OBJ_ARC, &p1, &p2, &p3) == 0)
					return 0;
				o = p2;
				replace=1;
				pcb_snprintf(buff, sizeof(buff), "%s %ld", find_rev_type(argv[by].type), (long)o->ID);
			}
			break;
		default:
			return 0;
	}

	if (*buff == '\0') {
		pcb_trace("nope...\n");
		return 0;
	}

	if (replace) {
		pcb_trace(" replace %d: '%s'\n", argn, buff);
		cli_str_remove(line, argv[argn].begin, argv[argn].end);
		cursor = cli_str_insert(line, argv[argn].begin, buff, 1);
		
	}
	else {
		pcb_trace(" insert-after %d: '%s'\n", argn, buff);
		cursor = cli_str_insert(line, argv[argn].end, buff, 1);
	}

pcb_trace("line='%s'\n", line);
	update:;
	pcb_hid_command_entry(line, &cursor);

	return 0;
}

static int line_tab(char *line, int cursor, int argc, cli_node_t *argv)
{
	pcb_trace("line t: '%s':%d\n", line, cursor);
	return -1;
}

