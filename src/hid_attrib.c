#include "config.h"
#include "hid_attrib.h"
#include "misc.h"
#include "misc_util.h"
#include "pcb-printf.h"
#include "compat_fs.h"
#include "error.h"

HID_AttrNode *hid_attr_nodes = 0;

void hid_register_attributes(HID_Attribute * a, int n, const char *cookie, int copy)
{
	HID_AttrNode *ha;

	/* printf("%d attributes registered\n", n); */
	ha = (HID_AttrNode *) malloc(sizeof(HID_AttrNode));
	ha->next = hid_attr_nodes;
	hid_attr_nodes = ha;
	ha->attributes = a;
	ha->n = n;
	ha->cookie = cookie;
}

void hid_attributes_uninit(void)
{
	HID_AttrNode *ha, *next;
	for (ha = hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie != NULL)
			fprintf(stderr, "WARNING: attribute %s by %s is not uninited, check your plugins' uninit!\n", ha->attributes->name, ha->cookie);
		free(ha);
	}
	hid_attr_nodes = NULL;
}

void hid_remove_attributes_by_cookie(const char *cookie)
{
	HID_AttrNode *ha, *next, *prev = NULL;
	for (ha = hid_attr_nodes; ha; ha = next) {
		next = ha->next;
		if (ha->cookie == cookie) {
			if (prev == NULL)
				hid_attr_nodes = next;
			else
				prev->next = next;
			free(ha);
		}
		else
			prev = ha;
	}
}

void hid_parse_command_line(int *argc, char ***argv)
{
	HID_AttrNode *ha;
	int i, e, ok;

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			HID_Attribute *a = ha->attributes + i;
			switch (a->type) {
			case HID_Label:
				break;
			case HID_Integer:
				if (a->value)
					*(int *) a->value = a->default_val.int_value;
				break;
			case HID_Coord:
				if (a->value)
					*(Coord *) a->value = a->default_val.coord_value;
				break;
			case HID_Boolean:
				if (a->value)
					*(char *) a->value = a->default_val.int_value;
				break;
			case HID_Real:
				if (a->value)
					*(double *) a->value = a->default_val.real_value;
				break;
			case HID_String:
				if (a->value)
					*(const char **) a->value = a->default_val.str_value;
				break;
			case HID_Enum:
				if (a->value)
					*(int *) a->value = a->default_val.int_value;
				break;
			case HID_Mixed:
				if (a->value) {
					*(HID_Attr_Val *) a->value = a->default_val;
			case HID_Unit:
					if (a->value)
						*(int *) a->value = a->default_val.int_value;
					break;
				}
				break;
			default:
				abort();
			}
		}

	while (*argc && (*argv)[0][0] == '-' && (*argv)[0][1] == '-') {
		int bool_val;
		int arg_ofs;

		bool_val = 1;
		arg_ofs = 2;
	try_no_arg:
		for (ha = hid_attr_nodes; ha; ha = ha->next)
			for (i = 0; i < ha->n; i++)
				if (strcmp((*argv)[0] + arg_ofs, ha->attributes[i].name) == 0) {
					HID_Attribute *a = ha->attributes + i;
					char *ep;
					const Unit *unit;
					switch (ha->attributes[i].type) {
					case HID_Label:
						break;
					case HID_Integer:
						if (a->value)
							*(int *) a->value = strtol((*argv)[1], 0, 0);
						else
							a->default_val.int_value = strtol((*argv)[1], 0, 0);
						(*argc)--;
						(*argv)++;
						break;
					case HID_Coord:
						if (a->value)
							*(Coord *) a->value = GetValue((*argv)[1], NULL, NULL, NULL);
						else
							a->default_val.coord_value = GetValue((*argv)[1], NULL, NULL, NULL);
						(*argc)--;
						(*argv)++;
						break;
					case HID_Real:
						if (a->value)
							*(double *) a->value = strtod((*argv)[1], 0);
						else
							a->default_val.real_value = strtod((*argv)[1], 0);
						(*argc)--;
						(*argv)++;
						break;
					case HID_String:
						if (a->value)
							*(char **) a->value = (*argv)[1];
						else
							a->default_val.str_value = (*argv)[1];
						(*argc)--;
						(*argv)++;
						break;
					case HID_Boolean:
						if (a->value)
							*(char *) a->value = bool_val;
						else
							a->default_val.int_value = bool_val;
						break;
					case HID_Mixed:
						a->default_val.real_value = strtod((*argv)[1], &ep);
						goto do_enum;
					case HID_Enum:
						ep = (*argv)[1];
					do_enum:
						ok = 0;
						for (e = 0; a->enumerations[e]; e++)
							if (strcmp(a->enumerations[e], ep) == 0) {
								ok = 1;
								a->default_val.int_value = e;
								a->default_val.str_value = ep;
								break;
							}
						if (!ok) {
							fprintf(stderr, "ERROR:  \"%s\" is an unknown value for the --%s option\n", (*argv)[1], a->name);
							exit(1);
						}
						(*argc)--;
						(*argv)++;
						break;
					case HID_Path:
						abort();
						a->default_val.str_value = (*argv)[1];
						(*argc)--;
						(*argv)++;
						break;
					case HID_Unit:
						unit = get_unit_struct((*argv)[1]);
						if (unit == NULL) {
							fprintf(stderr, "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n", (*argv)[1], a->name);
							exit(1);
						}
						a->default_val.int_value = unit->index;
						a->default_val.str_value = unit->suffix;
						(*argc)--;
						(*argv)++;
						break;
					}
					(*argc)--;
					(*argv)++;
					ha = 0;
					goto got_match;
				}
		if (bool_val == 1 && strncmp((*argv)[0], "--no-", 5) == 0) {
			bool_val = 0;
			arg_ofs = 5;
			goto try_no_arg;
		}
		fprintf(stderr, "unrecognized option: %s\n", (*argv)[0]);
		exit(1);
	got_match:;
	}
}

void hid_usage_option(const char *name, const char *help)
{
	fprintf(stderr, "%-20s %s\n", name, help);
}

void hid_usage(HID_Attribute * a, int numa)
{
	for (; numa > 0; numa--,a++) {
		const char *help;
		if (a->help_text == NULL) help = "";
		else if (a->help_text == ATTR_UNDOCUMENTED) help = "<undocumented>";
		else help = a->help_text;
		hid_usage_option(a->name, help);
	}
}
