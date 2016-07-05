#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "conf.h"

int lineno = 0;

void Message(const char *Format, ...)
{
	va_list args;

	fprintf(stderr, "Error in line %d: ", lineno);

	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);

	fprintf(stderr, "\n");
}

void hid_notify_conf_changed(void)
{

}

void hid_usage_option(const char *name, const char *help)
{
}

extern lht_doc_t *conf_root[];

void cmd_dump(char *arg)
{
	if (strncmp(arg, "native", 6) == 0) {
		arg+=7;
		while(isspace(*arg)) arg++;
		conf_dump(stdout, "", 1, arg);
	}
	else if (strncmp(arg, "lihata", 6) == 0) {
		arg+=7;
		while(isspace(*arg)) arg++;
		conf_role_t role = conf_role_parse(arg);
		if (role == CFR_invalid) {
			Message("Invalid role: '%s'", arg);
			return;
		}
		if (conf_root[role] != NULL)
			lht_dom_export(conf_root[role]->root, stdout, "");
		else
			printf("<empty>\n");
	}
	else
		Message("Invalid dump mode: '%s'", arg);
}

int main()
{
	char line[1024];

	conf_init();
	conf_core_init();
	conf_core_postproc();
	conf_load_all(NULL, NULL);

	while(fgets(line, sizeof(line), stdin)) {
		char *end = line + strlen(line) - 1;
		char *cmd, *arg;

		lineno++;

		while((end >= line) && ((*end == '\n') || (*end == '\r'))) {
			*end = '\0';
			end--;
		}
		cmd = line;
		while(isspace(*cmd)) cmd++;
		if (*cmd == '#')
			continue;
		arg = strpbrk(cmd, " \t");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
			while(isspace(*arg)) arg++;
		}

		if (strcmp(cmd, "dump") == 0)
			cmd_dump(arg);
		else
			Message("unknown command '%s'", cmd);
	}

	conf_uninit();
}
