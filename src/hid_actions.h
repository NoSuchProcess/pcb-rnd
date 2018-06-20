#ifndef PCB_HID_ACTIONS_H
#define PCB_HID_ACTIONS_H

#include "hid.h"

/* These are called from main_act.c */
void pcb_print_actions(void);
void pcb_dump_actions(void);

const pcb_hid_action_t *pcb_hid_find_action(const char *name);

extern void pcb_hid_remove_actions(const pcb_hid_action_t * a, int n);
extern void pcb_hid_remove_action(const pcb_hid_action_t * a);
extern void pcb_hid_remove_actions_by_cookie(const char *cookie);

int pcb_hid_action(const char *action_);
int pcb_hid_actionl(const char *action_, ...);	/* NULL terminated */
int pcb_hid_actionv(const char *action_, int argc_, const char **argv_);
int pcb_hid_actionv_(const pcb_hid_action_t *a, int argc, const char **argv);

/* Parse the given command string into action calls, and call
   hid_actionv for each action found.  Accepts both "action(arg1,
   arg2)" and command-style "action arg1 arg2", allowing only one
   action in the later case.  Returns nonzero if the action handler(s)
   return nonzero. */
int pcb_hid_parse_command(const char *str_);

/* Parse the given string into action calls, and call
   hid_actionv for each action found.  Accepts only
   "action(arg1, arg2)" */
int pcb_hid_parse_actions(const char *str_);

/* If the mouse cursor is in the drawin area, set x;y silently and return;
   else show msg and let the user click in the drawing area */
void pcb_hid_get_coords(const char *msg, pcb_coord_t *x, pcb_coord_t *y);

#endif
