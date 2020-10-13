/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Query language - layer setup checks */

typedef enum { LSL_ON, LSL_BELOW, LSL_ABOVE, LSL_max } layer_setup_loc_t;
typedef enum { NONE = 0x1000, TYPE, NET, NETMARGIN } layer_setup_target_t;

typedef struct {
	/* for a composite check */
	pcb_layer_type_t require_lyt[LSL_max];
	pcb_layer_type_t refuse_lyt[LSL_max];
	pcb_net_t *require_net[LSL_max];
	rnd_coord_t net_margin[LSL_max];

	/* for an atomic query: loc|target */
	unsigned long loc_target;
} layer_setup_t;

void pcb_qry_uninit_layer_setup(pcb_qry_exec_t *ectx)
{
	if (!ectx->layer_setup_inited)
		return;

	genht_uninit_deep(htpp, &ectx->layer_setup_precomp, {
		free(htent->value);
	});

	ectx->layer_setup_inited = 0;
}

static void pcb_qry_init_layer_setup(pcb_qry_exec_t *ectx)
{
	htpp_init(&ectx->layer_setup_precomp, ptrhash, ptrkeyeq);
	ectx->layer_setup_inited = 1;
}

#define LSC_RESET() \
do { \
	val = NULL; \
	loc = LSL_ON; \
	target = NONE; \
} while(0)

#define LSC_GET_VAL(err_stmt) \
do { \
	len = s-val; \
	if ((val == NULL) || (len >= sizeof(tmp))) { err_stmt; } \
	strncpy(tmp, val, len); \
	tmp[len] = '\0'; \
} while(0)

static long layer_setup_compile_(pcb_qry_exec_t *ectx, layer_setup_t *ls, const char *s_, rnd_bool composite)
{
	const char *val;
	layer_setup_loc_t loc;
	layer_setup_target_t target;
	const char *s;
	char tmp[1024];
	int len;
	rnd_bool succ;
	static const pcb_layer_type_t lyt_allow = PCB_LYT_COPPER | PCB_LYT_SILK | PCB_LYT_MASK | PCB_LYT_PASTE;
	pcb_net_t *net;

	LSC_RESET();

	/* above-type:air,below-net:gnd;below-netmargin:10mil */
	for(s = s_;;) {
		while(isspace(*s) || (*s == '-')) s++;
		if ((*s == '\0') || (*s == ',') || (*s == ';') || (*s == ':')) {
			ls->loc_target = loc | target;

			if (*s == ':') {
				s++;
				val = s;
				while((*s != '\0') && (*s != ',') && (*s != ';')) s++;
			}

			if (!composite)
				return 0;

			/* close current rule */
			switch(target) {
				case TYPE:
					LSC_GET_VAL({
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid layer type value '%s'\n", val == NULL ? s_ : val);
						return -1;
					});
					if (strcmp(tmp, "air") == 0)
						ls->refuse_lyt[loc] = PCB_LYT_COPPER | PCB_LYT_MASK | PCB_LYT_PASTE;
					else if (strcmp(tmp, "noncopper") == 0)
						ls->refuse_lyt[loc] = PCB_LYT_COPPER;
					else {
						pcb_layer_type_t lyt = pcb_layer_type_str2bit(val);
						if ((lyt == -1) || ((lyt & lyt_allow) != lyt)) {
							rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid netmargin value '%s'\n", val);
							return -1;
						}
						ls->require_lyt[loc] = lyt;
					}
					break;

				case NET:
					LSC_GET_VAL({
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid net name '%s'\n", val == NULL ? s_ : val);
						return -1;
					});
					net = pcb_net_get(ectx->pcb, &ectx->pcb->netlist[PCB_NETLIST_EDITED], tmp, 0);
					if (net == NULL) {
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: net not found: '%s'\n", val);
						return -1;
					}
					ls->require_net[loc] = net;
					break;

				case NETMARGIN:
					LSC_GET_VAL(goto err_coord);
					ls->net_margin[loc] = rnd_get_value(tmp, NULL, NULL, &succ);
					if (!succ) {
						err_coord:;
						rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid netmargin value '%s'\n", val);
						return -1;
					}
					break;
				case NONE:
					rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: missing target in '%s'\n", s_);
					return -1;
			}

			/* continue with the next rule */
			LSC_RESET();
			if (*s == '\0')
				break;
			s++;
			continue;
		}



		if (strncmp(s, "above", 5) == 0)           { s += 5; loc = LSL_ABOVE; }
		else if (strncmp(s, "below", 5) == 0)      { s += 5; loc = LSL_BELOW; }
		else if (strncmp(s, "netmargin", 9) == 0)  { s += 9; target = NETMARGIN; }
		else if (strncmp(s, "type", 4) == 0)       { s += 4; target = TYPE; }
		else if (strncmp(s, "net", 3) == 0)        { s += 3; target = NET; }
		else if (strncmp(s, "on", 2) == 0)         { s += 2; loc = LSL_ON; }
		else {
			rnd_message(RND_MSG_ERROR, "layer_setup() compilation error: invalid target or location in '%s'\n", s);
			return -1;
		}
	}

	return 0;
}

/* Cache access wrapper around compile - assume the same string will have the same pointer (e.g. net attribute) */
static const layer_setup_t *layer_setup_compile(pcb_qry_exec_t *ectx, const char *s, rnd_bool composite)
{
	layer_setup_t *ls;
	ls = htpp_get(&ectx->layer_setup_precomp, s);
	if (ls != NULL)
		return ls;

	ls = calloc(sizeof(layer_setup_t), 1);
	layer_setup_compile_(ectx, ls, s, composite);
	htpp_set(&ectx->layer_setup_precomp, (void *)s, ls);
	return ls;
}

static int fnc_layer_setup(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj, *lo;
	pcb_layer_t *ly = NULL;
	const char *lss;
	const layer_setup_t *ls;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_STRING))
		return -1;

	lss = argv[1].data.str;

	if (!ectx->layer_setup_inited)
		pcb_qry_init_layer_setup(ectx);

	obj = (pcb_any_obj_t *)argv[0].data.obj;
	ls = layer_setup_compile(ectx, lss, 1);
rnd_trace("layer_setup: %p/'%s' -> %p\n", lss, lss, ls);

	PCB_QRY_RET_INV(res);
}

