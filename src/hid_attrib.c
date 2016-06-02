#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "global.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "misc.h"
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

	(*argc)--;
	(*argv)++;

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
							*(Coord *) a->value = GetValue((*argv)[1], NULL, NULL);
						else
							a->default_val.coord_value = GetValue((*argv)[1], NULL, NULL);
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

	(*argc)++;
	(*argv)--;
}

static int attr_hash(HID_Attribute * a)
{
	unsigned char *cp = (unsigned char *) a;
	int i, rv = 0;
	for (i = 0; i < (int) ((char *) &(a->hash) - (char *) a); i++)
		rv = (rv * 13) ^ (rv >> 16) ^ cp[i];
	return rv;
}

#warning TODO: this should be removed among with hid_load_settings*
void hid_save_settings(int locally)
{
	char *fname;
	struct stat st;
	FILE *f;
	HID_AttrNode *ha;
	int i;

	if (locally) {
		fname = Concat("pcb.settings", NULL);
	}
	else {
		if (homedir == NULL)
			return;
		fname = Concat(homedir, PCB_DIR_SEPARATOR_S, ".pcb", NULL);

		if (stat(fname, &st))
			if (MKDIR(fname, 0777)) {
				free(fname);
				return;
			}
		free(fname);

		fname = Concat(homedir, PCB_DIR_SEPARATOR_S, ".pcb", PCB_DIR_SEPARATOR_S, "settings", NULL);
	}

	f = fopen(fname, "w");
	if (!f) {
		Message("Can't open %s", fname);
		free(fname);
		return;
	}

	for (ha = hid_attr_nodes; ha; ha = ha->next) {
		for (i = 0; i < ha->n; i++) {
			const char *str;
			HID_Attribute *a = ha->attributes + i;

			if (a->hash == attr_hash(a))
				fprintf(f, "# ");
			switch (a->type) {
			case HID_Label:
				break;
			case HID_Integer:
				fprintf(f, "%s = %d\n", a->name, a->value ? *(int *) a->value : a->default_val.int_value);
				break;
			case HID_Coord:
				pcb_fprintf(f, "%s = %$mS\n", a->name, a->value ? *(Coord *) a->value : a->default_val.coord_value);
				break;
			case HID_Boolean:
				fprintf(f, "%s = %d\n", a->name, a->value ? *(char *) a->value : a->default_val.int_value);
				break;
			case HID_Real:
				fprintf(f, "%s = %f\n", a->name, a->value ? *(double *) a->value : a->default_val.real_value);
				break;
			case HID_String:
			case HID_Path:
				str = a->value ? *(char **) a->value : a->default_val.str_value;
				fprintf(f, "%s = %s\n", a->name, str ? str : "");
				break;
			case HID_Enum:
				fprintf(f, "%s = %s\n", a->name, a->enumerations[a->value ? *(int *) a->value : a->default_val.int_value]);
				break;
			case HID_Mixed:
				{
					HID_Attr_Val *value = a->value ? (HID_Attr_Val *) a->value : &(a->default_val);
					fprintf(f, "%s = %g%s\n", a->name, value->real_value, a->enumerations[value->int_value]);
				}
				break;
			case HID_Unit:
				fprintf(f, "%s = %s\n", a->name, get_unit_list()[a->value ? *(int *) a->value : a->default_val.int_value].suffix);
				break;
			}
		}
		fprintf(f, "\n");
	}
	fclose(f);
	free(fname);
}

static void hid_set_attribute(char *name, char *value)
{
	const Unit *unit;
	HID_AttrNode *ha;
	int i, e, ok;

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++)
			if (strcmp(name, ha->attributes[i].name) == 0) {
				HID_Attribute *a = ha->attributes + i;
				switch (ha->attributes[i].type) {
				case HID_Label:
					break;
				case HID_Integer:
					a->default_val.int_value = strtol(value, 0, 0);
					break;
				case HID_Coord:
					a->default_val.coord_value = GetValue(value, NULL, NULL);
					break;
				case HID_Real:
					a->default_val.real_value = strtod(value, 0);
					break;
				case HID_String:
					a->default_val.str_value = strdup(value);
					break;
				case HID_Boolean:
					a->default_val.int_value = 1;
					break;
				case HID_Mixed:
					a->default_val.real_value = strtod(value, &value);
					/* fall through */
				case HID_Enum:
					ok = 0;
					for (e = 0; a->enumerations[e]; e++)
						if (strcmp(a->enumerations[e], value) == 0) {
							ok = 1;
							a->default_val.int_value = e;
							a->default_val.str_value = value;
							break;
						}
					if (!ok) {
						fprintf(stderr, "ERROR:  \"%s\" is an unknown value for the %s option\n", value, a->name);
						exit(1);
					}
					break;
				case HID_Path:
					a->default_val.str_value = value;
					break;
				case HID_Unit:
					unit = get_unit_struct(value);
					if (unit == NULL) {
						fprintf(stderr, "ERROR:  unit \"%s\" is unknown to pcb (option --%s)\n", value, a->name);
						exit(1);
					}
					a->default_val.int_value = unit->index;
					a->default_val.str_value = unit->suffix;
					break;
				}
			}
}

static void hid_load_settings_1(char *fname)
{
	char line[1024], *namep, *valp, *cp;
	FILE *f;

	f = fopen(fname, "r");
	if (!f) {
		free(fname);
		return;
	}

	free(fname);
	while (fgets(line, sizeof(line), f) != NULL) {
		for (namep = line; *namep && isspace((int) *namep); namep++);
		if (*namep == '#')
			continue;
		for (valp = namep; *valp && !isspace((int) *valp); valp++);
		if (!*valp)
			continue;
		*valp++ = 0;
		while (*valp && (isspace((int) *valp) || *valp == '='))
			valp++;
		if (!*valp)
			continue;
		cp = valp + strlen(valp) - 1;
		while (cp >= valp && isspace((int) *cp))
			*cp-- = 0;
		hid_set_attribute(namep, valp);
	}

	fclose(f);
}

void hid_load_settings()
{
	HID_AttrNode *ha;
	int i;

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++)
			ha->attributes[i].hash = attr_hash(ha->attributes + i);

	hid_load_settings_1(Concat(pcblibdir, PCB_DIR_SEPARATOR_S, "settings", NULL));
	if (homedir != NULL)
		hid_load_settings_1(Concat(homedir, PCB_DIR_SEPARATOR_S, ".pcb", PCB_DIR_SEPARATOR_S, "settings", NULL));
	hid_load_settings_1(Concat("pcb.settings", NULL));
}
