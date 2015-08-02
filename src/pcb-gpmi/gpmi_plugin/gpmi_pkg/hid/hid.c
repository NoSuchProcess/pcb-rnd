#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gpmi.h>
#include "src/global.h"
#include "src/hid.h"
#include "src/hid/hidint.h"
#include "src/pcb-printf.h"
#define FROM_PKG
#include "hid.h"
#include "hid_callbacks.h"


void hid_gpmi_data_set(hid_t *h, void *data)
{
#warning TODO
	/* Ugly workaround: as long as we don't have a data field in the hid,
	   structure, exporters, gui and gadgets can abuse this field */
	h->hid->calibrate = data;
}

hid_t *hid_gpmi_data_get(HID *h)
{
	if (h == NULL)
		return NULL;
	return (hid_t *)h->calibrate;
}

hid_t *hid_create(char *hid_name, char *description)
{
	hid_t *h;

	h      = calloc(sizeof(hid_t), 1);
	h->hid = calloc(sizeof(HID), 1);

	common_nogui_init (h->hid);

	h->module = gpmi_get_current_module();

	h->hid->name        = strdup(hid_name);
	h->hid->description = strdup(description);
	h->hid->exporter    = 1;
	h->hid->gui         = 0;
	h->hid->struct_size = sizeof(HID);

	h->hid->get_export_options = gpmi_hid_get_export_options;
	h->hid->make_gc            = gpmi_hid_make_gc;
	h->hid->destroy_gc         = gpmi_hid_destroy_gc;
	h->hid->do_export          = gpmi_hid_do_export;
	h->hid->parse_arguments    = gpmi_hid_parse_arguments;
	h->hid->set_crosshair      = gpmi_hid_set_crosshair;
	h->hid->set_layer          = gpmi_hid_set_layer;
	h->hid->set_color          = gpmi_hid_set_color;
	h->hid->set_line_cap       = gpmi_hid_set_line_cap;
	h->hid->set_line_width     = gpmi_hid_set_line_width;
	h->hid->set_draw_xor       = gpmi_hid_set_draw_xor;
	h->hid->set_draw_faded     = gpmi_hid_set_draw_faded;
	h->hid->draw_line          = gpmi_hid_draw_line;
	h->hid->draw_arc           = gpmi_hid_draw_arc;
	h->hid->draw_rect          = gpmi_hid_draw_rect;
	h->hid->fill_circle        = gpmi_hid_fill_circle;
	h->hid->fill_polygon       = gpmi_hid_fill_polygon;
	h->hid->fill_pcb_polygon   = gpmi_hid_fill_pcb_polygon;
	h->hid->fill_rect          = gpmi_hid_fill_rect;
	h->hid->use_mask           = gpmi_hid_use_mask;

	h->attr_num = 0;
	h->attr     = NULL;
	h->new_gc   = NULL;

	hid_gpmi_data_set(h, h);
	return h;
}

dynamic char *hid_get_attribute(hid_t *hid, int attr_id)
{
	char *res;
	char buff[128];
	HID_Attr_Val *v;

	if ((hid == NULL) || (attr_id < 0) || (attr_id >= hid->attr_num) || (hid->result == NULL))
		return 0;

	res = NULL;

	v = &(hid->result[attr_id]);
	switch(hid->type[attr_id]) {
		case HIDA_Boolean:
			if (v->int_value)
				res = "true";
			else
				res = "false";
			break;
		case HIDA_Integer:
			snprintf(buff, sizeof(buff), "%d", v->int_value);
			res = buff;
			break;
		case HIDA_Real:
			snprintf(buff, sizeof(buff), "%f", v->real_value);
			res = buff;
			break;
		case HIDA_String:
		case HIDA_Label:
		case HIDA_Enum:
		case HIDA_Path:
			res = v->str_value;
			break;
		case HIDA_Coord:
				pcb_sprintf(buff, "%mi", v->coord_value);
				res = buff;
				break;
		case HIDA_Unit:
			{
				const Unit *u;
				double fact;
				u = get_unit_by_idx(v->int_value);
				if (u == NULL)
					fact = 0;
				else
					fact = unit_to_factor(u);
				snprintf(buff, sizeof(buff), "%f", fact);
				res = buff;
/*				fprintf(stderr, "unit idx: %d %p res='%s'\n", v->int_value, u, res);*/
			}
			break;
		case HIDA_Mixed:
		default:
			fprintf(stderr, "error: hid_string2val: can't handle type %d\n", hid->type[attr_id]);

	}
	if (res == NULL)
		return NULL;
	return strdup(res);
}


HID_Attr_Val hid_string2val(const hid_attr_type_t type, const char *str)
{
	HID_Attr_Val v;
	memset(&v, 0, sizeof(v));
	switch(type) {
		case HIDA_Boolean:
			if ((strcasecmp(str, "true") == 0) || (strcasecmp(str, "yes") == 0) || (strcasecmp(str, "1") == 0))
				v.int_value = 1;
			else
				v.int_value = 0;
			break;
		case HIDA_Integer:
			v.int_value = atoi(str);
			break;
		case HIDA_Coord:
			{
				char *end;
				double val;
				val = strtod(str, &end);
				while(isspace(*end)) end++;
				if (*end != '\0') {
					const Unit *u;
					u = get_unit_struct(end);
					if (u == NULL) {
						Message("Invalid unit for HIDA_Coord in the script: '%s'\n", end);
						v.coord_value = 0;
					}
					else
						v.coord_value = unit_to_coord(u, val);
				}
				else 
					v.coord_value = val;
			}
			break;
		case HIDA_Unit:
			{
				const Unit *u;
				u = get_unit_struct(str);
				if (u != NULL)
					v.real_value = unit_to_factor(u);
				else
					v.real_value = 0;
			}
			break;
		case HIDA_Real:
			v.real_value = atof(str);
			break;
		case HIDA_String:
		case HIDA_Label:
		case HIDA_Enum:
		case HIDA_Path:
			v.str_value = strdup(str);
			break;
		case HIDA_Mixed:
		default:
			fprintf(stderr, "error: hid_string2val: can't handle type %d\n", type);
	}
	return v;
}

char **hid_string2enum(const char *str, HID_Attr_Val *def)
{
	char **e, *s, *last;
	int n, len;

	for(n=0, s=str; *s != '\0'; s++)
		if (*s == '|')
			n++;
	e = malloc(sizeof(char *) * (n+2));

	def->int_value = 0;
	def->str_value = NULL;
	def->real_value = 0.0;

	for(n = 0, last=s=str;; s++) {

		if ((*s == '|') || (*s == '\0')) {
			if (*last == '*') {
				def->int_value = n;
				last++;
			}
			len = s - last;
			e[n] = malloc(len+1);
			if (len != 0)
				strncpy(e[n], last, len);
			e[n][len] = '\0';
			last = s+1;
			n++;
		}
		if (*s == '\0')
			break;
	}
	e[n] = NULL;
	return e;
}

int hid_add_attribute(hid_t *hid, char *attr_name, char *help, hid_attr_type_t type, int min, int max, char *default_val)
{
	int current = hid->attr_num;

	/* TODO: should realloc more space here */
	hid->attr_num++;
	hid->attr = realloc(hid->attr, sizeof(HID_Attribute) * hid->attr_num);
	hid->type = realloc(hid->type, sizeof(hid_attr_type_t) * hid->attr_num);

	hid->attr[current].name         = strdup(attr_name);
	hid->attr[current].help_text    = strdup(help);
	hid->attr[current].type         = type;
	hid->attr[current].min_val      = min;
	hid->attr[current].max_val      = max;
	if (type == HIDA_Unit) {
		const Unit *u, *all;
		all = get_unit_list();
		u = get_unit_struct(default_val);
		if (u != NULL)
			hid->attr[current].default_val.int_value = u-all;
		else
			hid->attr[current].default_val.int_value = -1;
	}
	else if (type == HIDA_Enum) {
		hid->attr[current].enumerations = hid_string2enum(default_val, &(hid->attr[current].default_val));
	}
	else {
		hid->attr[current].default_val  = hid_string2val(type, default_val);
		hid->attr[current].enumerations = NULL;
	}
	hid->attr[current].hash         = 0;

	hid->type[current] = type;

	return current;
}

int hid_register(hid_t *hid)
{
	hid_register_hid(hid->hid);
	return 0;
}
