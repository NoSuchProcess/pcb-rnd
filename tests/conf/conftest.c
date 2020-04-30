#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "config.h"
#include <librnd/core/hid.h>
#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/compat_misc.h>
#include "conf_core.h"
#include "src_plugins/diag/diag_conf.h"

int lineno = 0;
int global_notify = 0;
rnd_conf_hid_id_t hid_id;
const char *hid_cookie = "conftest cookie";
rnd_hid_t *rnd_gui = NULL;
const char *rnd_hidlib_default_embedded_menu = "";
const char *rnd_menu_file_paths[] = { "./", "~/.pcb-rnd/", PCBSHAREDIR "/", NULL };
const char *rnd_menu_name_fmt = "pcb-menu-%s.lht";

#define CONF_USER_DIR "~/" DOT_PCB_RND
const char *rnd_conf_userdir_path = CONF_USER_DIR;
const char *rnd_pcphl_conf_user_path = CONF_USER_DIR "/pcb-conf.lht";
const char *rnd_conf_sysdir_path = PCBSHAREDIR;
const char *rnd_conf_sys_path = PCBSHAREDIR "/pcb-conf.lht";

void pcbhl_conf_postproc(void)
{
}

const char *pcb_board_get_filename(void)
{
	return "dummy_brd.lht";
}

const char *pcb_board_get_name(void)
{
	return "dummy_brd";
}

int rnd_file_loaded_set_at(const char *catname, const char *name, const char *path, const char *desc)
{
	return 0;
}

int rnd_file_loaded_del_at(const char *catname, const char *name)
{
	return 0;
}


void watch_pre(rnd_conf_native_t *cfg, int idx)
{
	printf("watch_pre:  '%s' old value\n", cfg->hash_path);
}

void watch_post(rnd_conf_native_t *cfg, int idx)
{
	printf("watch_post: '%s' new value\n", cfg->hash_path);
}

void notify_pre(rnd_conf_native_t *cfg, int idx)
{
	if (global_notify)
		printf("notify_pre:  '%s' old value\n", cfg->hash_path);
}

void notify_post(rnd_conf_native_t *cfg, int idx)
{
	if (global_notify)
		printf("notify_post: '%s' new value\n", cfg->hash_path);
}

rnd_conf_hid_callbacks_t watch_cbs = {watch_pre, watch_post, NULL, NULL};
rnd_conf_hid_callbacks_t global_cbs = {notify_pre, notify_post, NULL, NULL};


extern lht_doc_t *pcb_conf_main_root[];
void cmd_dump(char *arg)
{
	if (arg == NULL) {
		rnd_message(RND_MSG_ERROR, "Need an arg: native or lihata");
		return;
	}
	if (strncmp(arg, "native", 6) == 0) {
		arg+=7;
		while(isspace(*arg)) arg++;
		conf_dump(stdout, "", 1, arg);
	}
	else if (strncmp(arg, "lihata", 6) == 0) {
		rnd_conf_role_t role;
		arg+=7;
		while(isspace(*arg)) arg++;
		role = rnd_conf_role_parse(arg);
		if (role == RND_CFR_invalid) {
			rnd_message(RND_MSG_ERROR, "Invalid role: '%s'", arg);
			return;
		}
		if (pcb_conf_main_root[role] != NULL)
			lht_dom_export(pcb_conf_main_root[role]->root, stdout, "");
		else
			printf("<empty>\n");
	}
	else
		rnd_message(RND_MSG_ERROR, "Invalid dump mode: '%s'", arg);
}

void cmd_print(char *arg)
{
	rnd_conf_native_t *node;
	gds_t s;

	if (arg == NULL) {
		rnd_message(RND_MSG_ERROR, "Need an arg: a native path");
		return;
	}
	node = rnd_conf_get_field(arg);
	if (node == NULL) {
		rnd_message(RND_MSG_ERROR, "No such path: '%s'", arg);
		return;
	}
	gds_init(&s);
	rnd_conf_print_native((rnd_conf_pfn)rnd_append_printf, &s, NULL, 0, node);
	printf("%s='%s'\n", node->hash_path, s.array);
	gds_uninit(&s);
}

void cmd_load(char *arg, int is_text)
{
	char *fn;
	rnd_conf_role_t role ;

	if (arg == NULL) {
		help:;
		rnd_message(RND_MSG_ERROR, "Need 2 args: role and %s", (is_text ? "lihata text" : "file name"));
		return;
	}

	if (*arg == '*') {
		rnd_conf_load_all(NULL, NULL);
		return;
	}

	fn = strchr(arg, ' ');
	if (fn == NULL)
		goto help;
	*fn = '\0';
	fn++;
	while(isspace(*fn)) fn++;

	role = rnd_conf_role_parse(arg);
	if (role == RND_CFR_invalid) {
		rnd_message(RND_MSG_ERROR, "Invalid role: '%s'", arg);
		return;
	}
	printf("Result: %d\n", rnd_conf_load_as(role, fn, is_text));
	rnd_conf_update(NULL, -1);
}

rnd_conf_policy_t current_policy = RND_POL_OVERWRITE;
rnd_conf_role_t current_role = RND_CFR_DESIGN;

void cmd_policy(char *arg)
{
	rnd_conf_policy_t np = rnd_conf_policy_parse(arg);
	if (np == RND_POL_invalid)
		rnd_message(RND_MSG_ERROR, "Invalid/unknown policy: '%s'", arg);
	else
		current_policy = np;
}

void cmd_role(char *arg)
{
	rnd_conf_role_t nr = rnd_conf_role_parse(arg);
	if (nr == RND_CFR_invalid)
		rnd_message(RND_MSG_ERROR, "Invalid/unknown role: '%s'", arg);
	else
		current_role = nr;
}

void cmd_chprio(char *arg)
{
	char *end;
	int np = strtol(arg == NULL ? "" : arg, &end, 10);
	lht_node_t *first;

	if ((*end != '\0') || (np < 0)) {
		rnd_message(RND_MSG_ERROR, "Invalid integer prio: '%s'", arg);
		return;
	}
	first = rnd_conf_lht_get_first(current_role, 0);
	if (first != NULL) {
		char tmp[128];
		char *end;
		end = strchr(first->name, '-');
		if (end != NULL)
			*end = '\0';
		sprintf(tmp, "%s-%d", first->name, np);
		free(first->name);
		first->name = rnd_strdup(tmp);
		rnd_conf_update(NULL, -1);
	}
}

void cmd_chpolicy(char *arg)
{
	rnd_conf_policy_t np;
	lht_node_t *first;

	if (arg == NULL) {
		rnd_message(RND_MSG_ERROR, "need a policy", arg);
		return;
	}
	np = rnd_conf_policy_parse(arg);
	if (np == RND_POL_invalid) {
		rnd_message(RND_MSG_ERROR, "Invalid integer policy: '%s'", arg);
		return;
	}

	first = rnd_conf_lht_get_first(current_role, 0);
	if (first != NULL) {
		char tmp[128];
		char *end;
		end = strchr(first->name, '-');
		if (end != NULL) {
			sprintf(tmp, "%s%s", arg, end);
			free(first->name);
			first->name = rnd_strdup(tmp);
		}
		else {
			free(first->name);
			first->name = rnd_strdup(arg);
		}
		rnd_conf_update(NULL, -1);
	}
}

void cmd_set(char *arg)
{
	char *path, *val;
	int res;

	path = arg;
	val = strpbrk(path, " \t=");
	if (val == NULL) {
		rnd_message(RND_MSG_ERROR, "set needs a value");
		return;
	}
	*val = '\0';
	val++;
	while(isspace(*val) || (*val == '=')) val++;

	res = rnd_conf_set(current_role, path, -1, val, current_policy);
	if (res != 0)
		printf("set error: %d\n", res);
}

void cmd_watch(char *arg, int add)
{
	rnd_conf_native_t *n = rnd_conf_get_field(arg);
	if (n == NULL) {
		rnd_message(RND_MSG_ERROR, "unknown path");
		return;
	}
	rnd_conf_hid_set_cb(n, hid_id, (add ? &watch_cbs : NULL));
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
		rnd_conf_reset(current_role, "<cmd_reset current>");
	}
	else if (*arg == '*') {
		int n;
		for(n = 0; n < RND_CFR_max_real; n++)
			rnd_conf_reset(n, "<cmd_reset *>");
	}
	else {
		rnd_conf_role_t role = rnd_conf_role_parse(arg);
		if (role == RND_CFR_invalid) {
			rnd_message(RND_MSG_ERROR, "Invalid role: '%s'", arg);
			return;
		}
		rnd_conf_reset(role, "<cmd_reset role>");
	}
	rnd_conf_update(NULL, -1);
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

	hid_id = rnd_conf_hid_reg(hid_cookie, &global_cbs);

	rnd_conf_init();
	conf_core_init();
	rnd_hidlib_conf_init();
	rnd_conf_reset(RND_CFR_SYSTEM, "<main>");
	rnd_conf_reset(RND_CFR_USER, "<main>");

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
			rnd_message(RND_MSG_ERROR, "unknown command '%s'", cmd);
	}

	conf_core_uninit_pre();
	rnd_conf_hid_unreg(hid_cookie);
	rnd_conf_uninit();
	return 0;
}
