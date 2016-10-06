#include "global.h"

static const char query_action_syntax[] =
	"query(dump, expr) - dry run: compile and dump an expression\n"
	;
static const char query_action_help[] = "Perform various queries on PCB data.";

static int query_action(int argc, const char **argv, Coord x, Coord y)
{
	const char *cmd = argc > 0 ? argv[0] : 0;

	if (cmd == NULL) {
		return -1;
	}

	if (strcmp(cmd, "dump") == 0) {
		printf("Script: '%s'\n", argv[1]);
		pcb_qry_set_input(argv[1]);
		qry_parse();
		return 0;
	}

	return -1;
}

HID_Action query_action_list[] = {
	{"query", NULL, query_action,
	 query_action_help, query_action_syntax}
};

REGISTER_ACTIONS(query_action_list, NULL)

#include "dolists.h"
void query_action_reg(const char *cookie)
{
	REGISTER_ACTIONS(query_action_list, cookie)
}
