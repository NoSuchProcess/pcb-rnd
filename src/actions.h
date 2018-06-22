#ifndef PCB_ACTIONS_H
#define PCB_ACTIONS_H

#include "hid.h"
#include <libfungw/fungw.h>

struct pcb_action_s {
	const char *name; /* action command name */
	fgw_error_t (*trigger_cb)(int argc, const char **argv); /* Action implementation; if this returns non-zero, no further actions will be invoked for this key/mouse event. */
	const char *description;/* Short description (help text) */
	const char *syntax; /* Full allowed syntax; use \n to separate lines.  */
};

extern fgw_ctx_t pcb_fgw;


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

const pcb_action_t *pcb_find_action(const char *name);

void pcb_remove_actions(const pcb_action_t *a, int n);
void pcb_remove_actions_by_cookie(const char *cookie);

int pcb_action(const char *action_);
int pcb_actionl(const char *action_, ...); /* NULL terminated */
int pcb_actionv(const char *action_, int argc_, const char **argv_);
int pcb_actionv_(const pcb_action_t *a, int argc, const char **argv);

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

/* temporary hack for smooth upgrade to fungw based actions */
#define PCB_OLD_ACT_BEGIN \
{ \
	int argc = oargc; \
	const char **argv = oargv

#define PCB_OLD_ACT_END \
	(void)argc; \
	(void)argv; \
}

#endif
