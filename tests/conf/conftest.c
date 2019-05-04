#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "hid.h"
#include "conf.h"
#include "conf_hid.h"
#include "conf_core.h"
#include "hidlib_conf.h"
#include "compat_misc.h"
#include "src_plugins/diag/diag_conf.h"

int lineno = 0;
int global_notify = 0;
conf_hid_id_t hid_id;
const char *hid_cookie = "conftest cookie";
pcb_hid_t *pcb_gui = NULL;
const char *pcb_hidlib_default_embedded_menu = "";

void pcb_message(enum pcb_message_level level, const char *Format, ...)
{
	va_list args;

	fprintf(stderr, "Error in line %d: ", lineno);

	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);

	fprintf(stderr, "\n");
}

const char *pcb_board_get_filename(void)
{
	return "dummy_brd.lht";
}

const char *pcb_board_get_name(void)
{
	return "dummy_brd";
}

int pcb_file_loaded_set_at(const char *catname, const char *name, const char *path, const char *desc)
{
	return 0;
}

int pcb_file_loaded_del_at(const char *catname, const char *name)
{
	return 0;
}


void watch_pre(conf_native_t *cfg, int idx)
{
	printf("watch_pre:  '%s' old value\n", cfg->hash_path);
}

void watch_post(conf_native_t *cfg, int idx)
{
	printf("watch_post: '%s' new value\n", cfg->hash_path);
}

void notify_pre(conf_native_t *cfg, int idx)
{
	if (global_notify)
		printf("notify_pre:  '%s' old value\n", cfg->hash_path);
}

void notify_post(conf_native_t *cfg, int idx)
{
	if (global_notify)
		printf("notify_post: '%s' new value\n", cfg->hash_path);
}

conf_hid_callbacks_t watch_cbs = {watch_pre, watch_post, NULL, NULL};
conf_hid_callbacks_t global_cbs = {notify_pre, notify_post, NULL, NULL};


extern lht_doc_t *conf_main_root[];
void cmd_dump(char *arg)
{
	if (arg == NULL) {
		pcb_message(PCB_MSG_ERROR, "Need an arg: native or lihata");
		return;
	}
	if (strncmp(arg, "native", 6) == 0) {
		arg+=7;
		while(isspace(*arg)) arg++;
		conf_dump(stdout, "", 1, arg);
	}
	else if (strncmp(arg, "lihata", 6) == 0) {
		conf_role_t role;
		arg+=7;
		while(isspace(*arg)) arg++;
		role = conf_role_parse(arg);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", arg);
			return;
		}
		if (conf_main_root[role] != NULL)
			lht_dom_export(conf_main_root[role]->root, stdout, "");
		else
			printf("<empty>\n");
	}
	else
		pcb_message(PCB_MSG_ERROR, "Invalid dump mode: '%s'", arg);
}

void cmd_print(char *arg)
{
	conf_native_t *node;
	gds_t s;

	if (arg == NULL) {
		pcb_message(PCB_MSG_ERROR, "Need an arg: a native path");
		return;
	}
	node = conf_get_field(arg);
	if (node == NULL) {
		pcb_message(PCB_MSG_ERROR, "No such path: '%s'", arg);
		return;
	}
	gds_init(&s);
	conf_print_native((conf_pfn)pcb_append_printf, &s, NULL, 0, node);
	printf("%s='%s'\n", node->hash_path, s.array);
	gds_uninit(&s);
}

void cmd_load(char *arg, int is_text)
{
	char *fn;
	conf_role_t role ;

	if (arg == NULL) {
		help:;
		pcb_message(PCB_MSG_ERROR, "Need 2 args: role and %s", (is_text ? "lihata text" : "file name"));
		return;
	}

	if (*arg == '*') {
		conf_load_all(NULL, NULL);
		return;
	}

	fn = strchr(arg, ' ');
	if (fn == NULL)
		goto help;
	*fn = '\0';
	fn++;
	while(isspace(*fn)) fn++;

	role = conf_role_parse(arg);
	if (role == CFR_invalid) {
		pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", arg);
		return;
	}
	printf("Result: %d\n", conf_load_as(role, fn, is_text));
	conf_update(NULL, -1);
}

conf_policy_t current_policy = POL_OVERWRITE;
conf_role_t current_role = CFR_DESIGN;

void cmd_policy(char *arg)
{
	conf_policy_t np = conf_policy_parse(arg);
	if (np == POL_invalid)
		pcb_message(PCB_MSG_ERROR, "Invalid/unknown policy: '%s'", arg);
	else
		current_policy = np;
}

void cmd_role(char *arg)
{
	conf_role_t nr = conf_role_parse(arg);
	if (nr == CFR_invalid)
		pcb_message(PCB_MSG_ERROR, "Invalid/unknown role: '%s'", arg);
	else
		current_role = nr;
}

void cmd_chprio(char *arg)
{
	char *end;
	int np = strtol(arg == NULL ? "" : arg, &end, 10);
	lht_node_t *first;

	if ((*end != '\0') || (np < 0)) {
		pcb_message(PCB_MSG_ERROR, "Invalid integer prio: '%s'", arg);
		return;
	}
	first = conf_lht_get_first(current_role, 0);
	if (first != NULL) {
		char tmp[128];
		char *end;
		end = strchr(first->name, '-');
		if (end != NULL)
			*end = '\0';
		sprintf(tmp, "%s-%d", first->name, np);
		free(first->name);
		first->name = pcb_strdup(tmp);
		conf_update(NULL, -1);
	}
}

void cmd_chpolicy(char *arg)
{
	conf_policy_t np;
	lht_node_t *first;

	if (arg == NULL) {
		pcb_message(PCB_MSG_ERROR, "need a policy", arg);
		return;
	}
	np = conf_policy_parse(arg);
	if (np == POL_invalid) {
		pcb_message(PCB_MSG_ERROR, "Invalid integer policy: '%s'", arg);
		return;
	}

	first = conf_lht_get_first(current_role, 0);
	if (first != NULL) {
		char tmp[128];
		char *end;
		end = strchr(first->name, '-');
		if (end != NULL) {
			sprintf(tmp, "%s%s", arg, end);
			free(first->name);
			first->name = pcb_strdup(tmp);
		}
		else {
			free(first->name);
			first->name = pcb_strdup(arg);
		}
		conf_update(NULL, -1);
	}
}

void cmd_set(char *arg)
{
	char *path, *val;
	int res;

	path = arg;
	val = strpbrk(path, " \t=");
	if (val == NULL) {
		pcb_message(PCB_MSG_ERROR, "set needs a value");
		return;
	}
	*val = '\0';
	val++;
	while(isspace(*val) || (*val == '=')) val++;

	res = conf_set(current_role, path, -1, val, current_policy);
	if (res != 0)
		printf("set error: %d\n", res);
}

void cmd_watch(char *arg, int add)
{
	conf_native_t *n = conf_get_field(arg);
	if (n == NULL) {
		pcb_message(PCB_MSG_ERROR, "unknown path");
		return;
	}
	conf_hid_set_cb(n, hid_id, (add ? &watch_cbs : NULL));
}

void cmd_notify(char *arg)
{
	if (arg != NULL) {
		if (strcmp(arg, "on") == 0)
			global_notify = 1;
		else if (strcmp(arg, "off") == 0)
			global_notify = 0;
	}
	printf("Notify is %s\n", global_notify ? "on" : "off");
}

void cmd_echo(char *arg)
{
	if (arg != NULL)
		printf("%s\n", arg);
}

void cmd_reset(char *arg)
{
	if (arg == NULL) {
		conf_reset(current_role, "<cmd_reset current>");
	}
	else if (*arg == '*') {
		int n;
		for(n = 0; n < CFR_max_real; n++)
			conf_reset(n, "<cmd_reset *>");
	}
	else {
		conf_role_t role = conf_role_parse(arg);
		if (role == CFR_invalid) {
			pcb_message(PCB_MSG_ERROR, "Invalid role: '%s'", arg);
			return;
		}
		conf_reset(role, "<cmd_reset role>");
	}
	conf_update(NULL, -1);
}

extern void cmd_help(char *arg);


char line[8192];

/* returns 1 if there's more to read */
int getline_cont(FILE *f)
{
	char *end = line + strlen(line) - 1;
	int offs = 0, cont;

	if (feof(f))
		return 0;

	do {
		int remain = sizeof(line)-offs;
		assert(remain > 0);
		cont = 0;
		if (fgets(line+offs, remain, f)) {
			char *start = line+offs;
			int len = strlen(start);
			lineno++;
			end = start + len - 1;
			while((end >= start) && ((*end == '\n') || (*end == '\r'))) {
				*end = '\0';
				end--;
			}
			if ((end >= start) && (*end == '\\')) {
				cont = 1;
				*end = '\n';
			}
			offs += len-1;
		}
		else {
			if (offs == 0)
				return 0;
		}
	} while(cont);
	return 1;
}

int main()
{

	hid_id = conf_hid_reg(hid_cookie, &global_cbs);

	conf_init();
	conf_core_init();
	pcb_hidlib_conf_init();
	conf_reset(CFR_SYSTEM, "<main>");
	conf_reset(CFR_USER, "<main>");

	while(getline_cont(stdin)) {
		char *arg, *cmd = line;
		while(isspace(*cmd)) cmd++;
		if ((*cmd == '#') || (*cmd == '\0'))
			continue;
		arg = strpbrk(cmd, " \t");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
			while(isspace(*arg)) arg++;
		}

		if (strcmp(cmd, "dump") == 0)
			cmd_dump(arg);
		else if (strcmp(cmd, "print") == 0)
			cmd_print(arg);
		else if (strcmp(cmd, "load") == 0)
			cmd_load(arg, 0);
		else if (strcmp(cmd, "paste") == 0)
			cmd_load(arg, 1);
		else if (strcmp(cmd, "reset") == 0)
			cmd_reset(arg);
		else if (strcmp(cmd, "set") == 0)
			cmd_set(arg);
		else if (strcmp(cmd, "policy") == 0)
			cmd_policy(arg);
		else if (strcmp(cmd, "chprio") == 0)
			cmd_chprio(arg);
		else if (strcmp(cmd, "chpolicy") == 0)
			cmd_chpolicy(arg);
		else if (strcmp(cmd, "role") == 0)
			cmd_role(arg);
		else if (strcmp(cmd, "watch") == 0)
			cmd_watch(arg, 1);
		else if (strcmp(cmd, "unwatch") == 0)
			cmd_watch(arg, 0);
		else if (strcmp(cmd, "notify") == 0)
			cmd_notify(arg);
		else if (strcmp(cmd, "echo") == 0)
			cmd_echo(arg);
		else if (strcmp(cmd, "help") == 0)
			cmd_help(arg);
		else
			pcb_message(PCB_MSG_ERROR, "unknown command '%s'", cmd);
	}

	conf_hid_unreg(hid_cookie);
	conf_uninit();
	return 0;
}
