#ifndef PCB_ACTIONS_H
#define PCB_ACTIONS_H

#include "hid.h"
#include <libfungw/fungw.h>

struct pcb_action_s {
	const char *name; /* action command name */
	fgw_error_t (*trigger_cb)(fgw_arg_t *ores, int argc, fgw_arg_t *argv); /* Action implementation; if this returns non-zero, no further actions will be invoked for this key/mouse event. */
	const char *description;/* Short description (help text) */
	const char *syntax; /* Full allowed syntax; use \n to separate lines.  */
};

extern fgw_ctx_t pcb_fgw;

typedef struct fgw_coords_s {
	int len;
	pcb_coord_t c[4];
	char absolute[4];
} fgw_coords_t;

typedef enum {
	FGW_KEYWORD_ = FGW_CUSTOM,
	FGW_COORD_,
	FGW_COORDS_,
	FGW_LAYERID_,
	FGW_LAYER_
} pcb_fgw_types_e;
#define fgw_keyword(arg) ((arg)->val.nat_int)
#define fgw_coord(arg) (*(pcb_coord_t *)(&((arg)->val.custom.c)))
#define fgw_coords(arg) ((arg)->val.ptr_void)
#define fgw_layerid(arg) ((arg)->val.nat_long)
#define fgw_layer(arg) ((arg)->val.ptr_void)
#define FGW_KEYWORD ((fgw_type_t)FGW_KEYWORD_)
#define FGW_COORD   ((fgw_type_t)FGW_COORD_)
#define FGW_COORDS  ((fgw_type_t)FGW_COORDS_)
#define FGW_LAYERID  ((fgw_type_t)FGW_LAYERID_)
#define FGW_LAYER  ((fgw_type_t)FGW_LAYER_)

void pcb_register_action(const pcb_action_t *a, const char *cookie);
void pcb_register_actions(const pcb_action_t *a, int, const char *cookie);
#define PCB_REGISTER_ACTIONS(a, cookie) PCB_HIDCONCAT(void register_,a) ()\
	{ pcb_register_actions(a, sizeof(a)/sizeof(a[0]), cookie); }

/* Inits and uninits the whole action framework */
void pcb_actions_init(void);
void pcb_actions_uninit(void);

/* These are called from main_act.c */
void pcb_print_actions(void);
void pcb_dump_actions(void);

const pcb_action_t *pcb_find_action(const char *name, fgw_func_t **f_out);

void pcb_remove_actions(const pcb_action_t *a, int n);
void pcb_remove_actions_by_cookie(const char *cookie);

int pcb_action(const char *action_);
int pcb_actionl(const char *action_, ...); /* NULL terminated */
int pcb_actionv(const char *action_, int argc_, const char **argv_);
fgw_error_t pcb_actionv_(const fgw_func_t *f, fgw_arg_t *res, int argc, fgw_arg_t *argv);

/* Parse the given command string into action calls, and call
   hid_actionv for each action found.  Accepts both "action(arg1,
   arg2)" and command-style "action arg1 arg2", allowing only one
   action in the later case.  Returns nonzero if the action handler(s)
   return nonzero. */
int pcb_parse_command(const char *str_);

/* Parse the given string into action calls, and call
   hid_actionv for each action found.  Accepts only
   "action(arg1, arg2)" */
int pcb_parse_actions(const char *str_);

/* If the mouse cursor is in the drawin area, set x;y silently and return;
   else show msg and let the user click in the drawing area */
void pcb_hid_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y);

#define PCB_ACTION_MAX_ARGS 16

/* Return 0 on success after an action call */
PCB_INLINE int pcb_act_result(fgw_arg_t *res, fgw_error_t ret)
{
	if (ret != 0)
		return -1;

	if (fgw_argv_conv(&pcb_fgw, res, FGW_INT) != 0)
		return -1;

	return res->val.nat_int;
}

/* Call an action function directly, bypassing fungw; evaluates to an int
   that is 0 on success */
#define PCB_ACT_CALL_C(func, res, argc, argv) \
	pcb_act_result(res, func(res, argc, argv))

/* Require argument idx to exist and convert it to type; on success, also execute stmt */
#define PCB_ACT_CONVARG(idx, type, aname, stmt) \
do { \
	if (argc <= idx) { \
		PCB_ACT_FAIL(aname); \
		return FGW_ERR_ARGC; \
	} \
	if (fgw_argv_conv(&pcb_fgw, &argv[idx], type) != 0) { \
		PCB_ACT_FAIL(aname); \
		return FGW_ERR_ARG_CONV; \
	} \
	{ stmt; } \
} while(0)

/* If argument idx exists, convert it to type; on success, also execute stmt */
#define PCB_ACT_MAY_CONVARG(idx, type, aname, stmt) \
do { \
	if (argc > idx) { \
		if (fgw_argv_conv(&pcb_fgw, &argv[idx], type) != 0) { \
			PCB_ACT_FAIL(aname); \
			return FGW_ERR_ARG_CONV; \
		} \
		{ stmt; } \
	} \
} while(0)

/* Set integer res value */
#define PCB_ACT_IRES(v) \
do { \
	res->type = FGW_INT; \
	res->val.nat_int = v; \
} while(0)

#define PCB_ACT_FAIL(x) { pcb_message(PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (pcb_acts_ ## x)); return FGW_ERR_ARG_CONV; }

/*** The default unit to use when a coord value doesn't have its own unit ***/
extern char *fgw_str2coord_unit; /* saved is char * too */
#define fgw_str2coord_unit_set(saved, newval) \
do { \
	saved = fgw_str2coord_unit; \
	fgw_str2coord_unit = newval; \
} while(0)

#define fgw_str2coord_unit_restore(saved) \
	fgw_str2coord_unit = saved

/*** temporary hack for smooth upgrade to fungw based actions ***/
PCB_INLINE int pcb_old_act_begin_conv(int oargc, fgw_arg_t *oargv, char **argv)
{
	int n;
	for(n = 0; n < PCB_ACTION_MAX_ARGS; n++)
		argv[n] = NULL;
	for(n = 1; n < oargc; n++) {
		if (fgw_argv_conv(&pcb_fgw, &oargv[n], FGW_STR) == 0)
			argv[n-1] = oargv[n].val.str;
		else
			argv[n-1] = "";
	}
	argv[n] = NULL;
	return oargc - 1;
}

#define PCB_OLD_ACT_BEGIN \
ores->type = FGW_INT; \
ores->val.nat_int = 0; \
{ \
	char *argv__[PCB_ACTION_MAX_ARGS]; \
	const char **argv = (const char **)argv__; \
	int argc = pcb_old_act_begin_conv(oargc, oargv, argv__)

#define PCB_OLD_ACT_END \
	(void)argc; \
	(void)argv; \
}

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define PCB_AFAIL(x) { pcb_message(PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (x##_syntax)); return FGW_ERR_ARG_CONV; }

#define PCB_ACTION_ARG(n) (argc > (n) ? argv[n] : NULL)

#endif
