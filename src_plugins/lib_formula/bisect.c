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

#include "config.h"
#include <librnd/core/actions.h>
#include "bisect.h"

#define CALL(value) \
do { \
	argv[specn].val.nat_double = value; \
	ferr = rnd_actionv_(f, &r, argc-2, argv+2); \
	if (ferr != 0) { \
		rnd_message(RND_MSG_ERROR, "formula_bisect: '%s' returned error on value %f\n", actname, value); \
		return -1; \
	} \
	fgw_arg_conv(&rnd_fgw, res, FGW_DOUBLE); \
	if (res->type != FGW_DOUBLE) { \
		rnd_message(RND_MSG_ERROR, "formula_bisect: '%s' returned not-a-number on value %f\n", actname, value); \
		return -1; \
	} \
} while(0)

const char pcb_acts_formula_bisect[] = "formula_bisect(action, res, args)";
const char pcb_acth_formula_bisect[] = "Find the value for exactly one of the arguments that produces the expected result. One argument must be a string with type:min:max:precision, the rest of the arguments and res must be numeric.";
fgw_error_t pcb_act_formula_bisect(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *actname, *spec;
	double target, min, max, err;
	fgw_arg_t r;
	int n, specn = 0;
	const fgw_func_t *f;
	fgw_error_t ferr;

	RND_ACT_CONVARG(1, FGW_STRING, formula_bisect, actname = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_DOUBLE, formula_bisect, target = argv[2].val.nat_double);
	for(n = 3; n < argc; n++) {
		if (((argv[n].type & FGW_STR) == FGW_STR) && (strchr(argv[n].val.str, ':') != NULL)) {
			if (specn > 0) {
				rnd_message(RND_MSG_ERROR, "formula_bisect: only one argument can be variable; the two are %d and %d now\n", specn, n);
				return FGW_ERR_ARG_CONV;
			}
			else
				specn = n;
		}
	}

	if (specn < 0) {
		rnd_message(RND_MSG_ERROR, "formula_bisect: one argument needs to be a variable specification string\n");
		return FGW_ERR_ARG_CONV;
	}

	f = rnd_act_lookup(actname);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "formula_bisect: no such action: %s\n", actname);
		return FGW_ERR_ARG_CONV;
	}


	spec = strchr(argv[specn].val.str, ':');
	if (sscanf(spec+1, "%lf:%lf:%lf", &min, &max, &err) != 3) {
		rnd_message(RND_MSG_ERROR, "formula_bisect: invalid range spec: '%s'\n", spec);
		return FGW_ERR_ARG_CONV;
	}


	fgw_arg_free(&rnd_fgw, &argv[specn]);
	argv[specn].type = FGW_DOUBLE;

	CALL(min);
	CALL(max);


	return 0;
}

