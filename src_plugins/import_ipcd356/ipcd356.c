
#include "config.h"

#include "board.h"
#include "data.h"
#include "safe_fs.h"

#include "hid.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"

static const char *ipcd356_cookie = "ipcd356 importer";

static int ipc356_parse(pcb_board_t *pcb, FILE *f, const char *fn)
{
	char line_[128], *line;
	int lineno = 0;

	while((line = fgets(line_, sizeof(line_), f)) != NULL) {
		lineno++;
		switch(*line) {
			case '\r':
			case '\n':
			case 'C': /* comment */
				break;
			case 'P': /* parameter */
			case '3': /* test feature */
				break;
			case '9': /* EOF */
				if ((line[1] == '9') && (line[2] == '9'))
					return 0;
				pcb_message(PCB_MSG_ERROR, "Invalid end-of-file marker in %s:%d - expected '999'\n", fn, lineno);
		}
	}

	pcb_message(PCB_MSG_ERROR, "Unexpected end of file - expected '999'\n");
	return 1;
}

static const char pcb_acts_LoadIpc356From[] = "LoadIpc356From(filename)";
static const char pcb_acth_LoadIpc356From[] = "Loads the specified IPC356-D netlist";
int pcb_act_LoadIpc356From(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	FILE *f;
	int res;

	f = pcb_fopen(argv[0], "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open %s for read\n", argv[0]);
		return 1;
	}
	res = ipc356_parse(PCB, f, argv[0]);
	fclose(f);
	return res;
}

pcb_hid_action_t import_ipcd356_action_list[] = {
	{"LoadIpc356From", 0, pcb_act_LoadIpc356From, pcb_acth_LoadIpc356From, pcb_acts_LoadIpc356From}
};
PCB_REGISTER_ACTIONS(import_ipcd356_action_list, ipcd356_cookie)

int pplg_check_ver_import_ipcd356(int ver_needed) { return 0; }

void pplg_uninit_import_ipcd356(void)
{
	pcb_hid_remove_actions_by_cookie(ipcd356_cookie);
}

#include "dolists.h"
int pplg_init_import_ipcd356(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(import_ipcd356_action_list, ipcd356_cookie);
	return 0;
}
