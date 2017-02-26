/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* GUI for parametric footprints parameter exploration */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hid_attrib.h"
#include "compat_misc.h"

#include "dlg_library_param.h"
#include "dlg_attribute.h"

#define MAX_PARAMS 128

static pcb_hid_attribute_t *attr_append(const char *name, pcb_hid_attribute_t *attrs, int *numattr)
{
	pcb_hid_attribute_t *a;

	if (*numattr >= MAX_PARAMS)
		return NULL;

	a = &attrs[*numattr];
	memset(a, 0, sizeof(pcb_hid_attribute_t));
	a->name = name;
	a->type = HID_String;

	(*numattr)++;
	return a;
}

#define used min_val
#define alloced max_val
static void append_enum(pcb_hid_attribute_t *a, const char *val)
{
	if (a->used+1 >= a->alloced) {
		a->alloced += 16;
		a->enumerations = realloc(a->enumerations, sizeof(char *) * a->alloced);
	}
	a->enumerations[a->used] = val;
	a->used++;
	a->enumerations[a->used] = NULL;
}
#undef used
#undef alloced

#define append(name) attr_append(pcb_strdup(name), attrs, &numattr);

#define colsplit() \
do { \
	col = strchr(col, ':'); \
	if (col != NULL) { \
		*col = '\0'; \
		col++; \
	} \
} while(0)

#define append_strdup(dst, src, seps) \
do { \
	if (src != NULL) { \
		char *__old__ = dst; \
		if (dst != NULL) { \
			dst = pcb_strdup_printf("%s%s%s", __old__, seps, src); \
			free(__old__); \
		} \
		else \
			dst = pcb_strdup(src); \
	} \
} while(0)

static void free_attr(pcb_hid_attribute_t *a)
{
	free((char *)a->name);
	free((char *)a->help_text);
	if (a->enumerations != NULL) {
		const char **s;
		for(s = a->enumerations; *s != NULL; s++)
			free((char *)*s);
		free(a->enumerations);
	}
}

static pcb_hid_attribute_t *find_attr(pcb_hid_attribute_t *attrs, int numattr, const char *name)
{
	int n;
	for(n = 0; n < numattr; n++)
		if (strcmp(attrs[n].name, name) == 0)
			return &attrs[n];
	return NULL;
}


static char *gen_cmd(char *fpname, pcb_hid_attribute_t *attrs, pcb_hid_attr_val_t *res, int numattr)
{
	int n, pushed = 0;
	gds_t sres;
	char *tmp;

	memset(&sres, 0, sizeof(sres));

	gds_append_str(&sres, fpname);

	/* cut original name at "(" */
	tmp = strchr(sres.array, '(');
	if (tmp != NULL)
		gds_truncate(&sres, tmp - sres.array);

	gds_append_str(&sres, "(");

	for(n = 0; n < numattr; n++) {
		char *desc, buff[128];
		const char *val;

		if (!attrs[n].changed)
			continue;

		switch(attrs[n].type) {
			case HID_Enum:
				val = attrs[n].enumerations[res[n].int_value];
				if (val != NULL) {
					desc = strstr((char *)val, " (");
					if (desc != NULL)
						*desc = '\0';
				}
				break;
			case HID_String:
				val = res[n].str_value;
				break;
			case HID_Coord:
				val = buff;
				pcb_snprintf(buff, sizeof(buff), "%$$mh", res[n].coord_value);
				break;
			default:;
		}

		if (val == NULL)
			continue;

		if (pushed)
			gds_append_str(&sres, ", ");
		pcb_append_printf(&sres, "%s=%s", attrs[n].name, val);
		pushed++;
	}

	gds_append_str(&sres, ")");
	return sres.array;
}

static void set_attr(pcb_hid_attribute_t *a, char *val)
{
	const char **s;
	char *desc;
	int vlen, len, n;

	switch(a->type) {
		case HID_Enum:
			vlen = strlen(val);
			for(n = 0, s = a->enumerations; *s != NULL; s++,n++) {
				desc = strstr(*s, " (");
				if (desc != NULL)
					len = desc - *s;
				else
					len = strlen(*s);
				if ((len == vlen) && (strncmp(*s, val, len) == 0)) {
					a->default_val.int_value = n;
					break;
				}
			}
			break;
		case HID_String:
			free((char *)a->default_val.str_value);
			a->default_val.str_value = pcb_strdup(val);
			break;
		case HID_Coord:
			a->default_val.coord_value = pcb_get_value(val, NULL, NULL, NULL);
			break;
		default:;
	}
}

static int param_split(char *buf, char *argv[], int amax)
{
	int n;
	char *next, *bump;

	for(n=0; ;n++) {
		if (n >= amax)
			return n+1;

		/* ltrim */
		while(isspace(*buf)) buf++;
		argv[n] = buf;

		/* find next param */
		bump = buf;
		next = strchr(buf, ',');
		if (next == NULL)
			return n+1;
		buf = next+1;

		/* rtrim */
		*next = '\0';
		next--;
		while((next >= bump) && (isspace(*next))) {
			*next = '\0';
			next--;
		}
	}
	return -1;
}


static void load_params(char *user_params, char *help_params, pcb_hid_attribute_t *attrs, int numattr)
{
	char *parain;
	char *parahlp;
	int argc_in, argc_help, posi, n;
	char *end, *argv_in[MAX_PARAMS], *argv_help[MAX_PARAMS];

	if (user_params == NULL)
		user_params = "";
	if (help_params == NULL)
		help_params = "";


	parain = pcb_strdup(user_params);
	parahlp = pcb_strdup(help_params);

	/* truncate trailing ")" */
	if (*parain != '\0') {
		end = parain + strlen(parain) - 1;
		if (*end == ')')
			*end = '\0';
	}

	argc_in = param_split(parain, argv_in, MAX_PARAMS);
	argc_help = param_split(parahlp, argv_help, MAX_PARAMS);

	/* iterate and assign default values and mark them changed to get them back */
	for(posi = n = 0; n < argc_in; n++) {
		char *key, *val, *sep;
		pcb_hid_attribute_t *a;

		sep = strchr(argv_in[n], '=');
		if (sep != NULL) {
			key = argv_in[n];
			*sep = '\0';
			val = sep+1;

			/* rtrim key */
			sep--;
			while((sep >= key) && (isspace(*sep))) {
				*sep = '\0';
				sep--;
			}

			/* ltrim val */
			while(isspace(*val)) val++;
		}
		else {
			if (posi >= argc_help) {
				pcb_message(PCB_MSG_ERROR, "More positional parameters than expected - ignoring %s", argv_in[n]);
				continue;
			}
			key = argv_help[posi];
			val = argv_in[n];
			posi++;
		}

		a = find_attr(attrs, numattr, key);
		if (a == NULL) {
			pcb_message(PCB_MSG_ERROR, "Unknown parameter %s - ignoring value %s", key, val);
			continue;
		}
		set_attr(a, val);
		a->changed = 1; /* make sure the value is kept on the outpute */
	}

	/* clean up */
	free(parain);
	free(parahlp);
}


char *pcb_gtk_library_param_ui(pcb_gtk_library_t *library_window, pcb_fplibrary_t *entry, const char *filter_txt)
{
	FILE *f;
	char *sres, *cmd, line[1024];
	pcb_hid_attribute_t *curr, attrs[MAX_PARAMS];
	pcb_hid_attr_val_t res[MAX_PARAMS];
	int n, numattr = 0;
	char *params = NULL, *descr = NULL;

	cmd = pcb_strdup_printf("%s --help", entry->data.fp.loc_info);
	f = popen(cmd, "r");
	free(cmd);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can not execute parametric footprint %s\n", entry->data.fp.loc_info);
		return NULL;
	}

	while(fgets(line, sizeof(line), f) > 0) {
		char *end, *col, *arg, *cmd = line;

		/* rtrim */
		end = line + strlen(line) - 1;
		while((end >= line) && ((*end == '\r') || (*end == '\n'))) {
			*end = '\0';
			end--;
		}

		/* ltrim */
		cmd = strchr(cmd, '@');
		if ((cmd == NULL) || (cmd[1] != '@'))
			continue;
		cmd+=2;
		arg = strpbrk(cmd, " \t\r\n");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
			while(isspace(*arg)) arg++;
		}
		col = cmd;

		/* parse */
		if (strcmp(cmd, "desc") == 0) {
			append_strdup(descr, arg, "\n");
		}
		else if (strcmp(cmd, "params") == 0) {
			params = pcb_strdup(arg);
		}
		else if (strncmp(cmd, "param:", 6) == 0) {
			colsplit();
			curr = append(col);
			curr->help_text = pcb_strdup(arg);
		}
		else if (strncmp(cmd, "dim:", 4) == 0) {
			curr->type = HID_Coord;
			curr->min_val = 0;
			curr->max_val = PCB_MM_TO_COORD(512);
		}
		else if (strncmp(cmd, "enum:", 5) == 0) {
			char *evl;
			curr->type = HID_Enum;
			colsplit(); colsplit();
			if (arg != NULL)
				evl = pcb_strdup_printf("%s (%s)", col, arg);
			else
				evl = pcb_strdup(col);
			append_enum(curr, evl);
		}
	}
	pclose(f);

	if (filter_txt != NULL) {
		char *prm = strchr(filter_txt, '(');
		if (prm != NULL)
			load_params(prm+1, params, attrs, numattr);
	}

	ghid_attribute_dialog(GTK_WINDOW_TOPLEVEL, attrs, numattr, res, "Parametric footprint edition", descr);

	sres = gen_cmd(entry->name, attrs, res, numattr);

	/* clean up */
	for(n = 0; n < numattr; n++)
		free_attr(&attrs[n]);
	free(descr);
	free(params);

	return sres;
}

