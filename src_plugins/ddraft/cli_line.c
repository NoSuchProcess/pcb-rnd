static int line_exec(char *line, int argc, cli_node_t *argv)
{
	int n;
	pcb_trace("line e: '%s'\n", line);
	for(n = 0; n < argc; n++) {
		pcb_trace(" [%d] %d\n", argv[n].type);
	}
	return -1;
}

static int line_click(char *line, int cursor, int argc, cli_node_t *argv)
{
	pcb_trace("line c: '%s':%d\n", line, cursor);
	return -1;
}

static int line_tab(char *line, int cursor, int argc, cli_node_t *argv)
{
	pcb_trace("line t: '%s':%d\n", line, cursor);
	return -1;
}

static int line_edit(char *line, int cursor, int argc, cli_node_t *argv)
{
	pcb_trace("line e: '%s':%d\n", line, cursor);
	return -1;
}
