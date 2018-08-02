static int line_parse(char *line, int argc, cli_node_t *argv, pcb_box_t *box, int verbose)
{
	int n = 0;

	if (argv[n].type == CLI_FROM)
		n++;
	else if (argv[n].type == CLI_TO) {
		pcb_message(PCB_MSG_ERROR, "Incremental line drawing is not yet supported\n");
		return -1;
	}
	n = cli_apply_coord(argc, argv, n, &box->X1, &box->Y1);
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
	n = cli_apply_coord(argc, argv, n, &box->X2, &box->Y2);
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
	cli_print_args(argc, argv);

	memset(&box, 0, sizeof(box));
	res = line_parse(line, argc, argv, &box, 1);
	if (res == 0) {
		pcb_trace("line_exec: %mm;%mm -> %mm;%mm\n", box.X1, box.Y1, box.X2, box.Y2);
		pcb_line_new(CURRENT, box.X1, box.Y1, box.X2, box.Y2,
			conf_core.design.line_thickness, 2 * conf_core.design.clearance,
			pcb_flag_make(conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0));
		pcb_route_reset(&pcb_crosshair.Route);
	}
	return res;
}

static int line_click(char *line, int cursor, int argc, cli_node_t *argv)
{
	pcb_trace("line c: '%s':%d\n", line, cursor);
	return 0;
}

static int line_tab(char *line, int cursor, int argc, cli_node_t *argv)
{
	pcb_trace("line t: '%s':%d\n", line, cursor);
	return -1;
}

static int line_edit(char *line, int cursor, int argc, cli_node_t *argv)
{
	int res;
	pcb_box_t box;

	if (pcb_tool_next_id != PCB_MODE_LINE)
		pcb_tool_select_by_id(PCB_MODE_LINE);

	pcb_trace("line e: '%s':%d\n", line, cursor);
	memset(&box, 0, sizeof(box));
	res = line_parse(line, argc, argv, &box, 0);
	if (res == 0) {
		pcb_route_object_t *line;
		if (pcb_crosshair.Route.size != 1)
			pcb_route_resize(&pcb_crosshair.Route, 1);
		pcb_crosshair.Route.thickness = 1;
		line = &pcb_crosshair.Route.objects[0];
		line->type = PCB_OBJ_LINE;
		line->point1.X = box.X1;
		line->point1.Y = box.Y1;
		line->point2.X = box.X2;
		line->point2.Y = box.Y2;
		line->layer = INDEXOFCURRENT;
		pcb_gui->invalidate_all();
	}
	return res;
}
