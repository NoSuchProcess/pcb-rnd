#ifndef PCB_HID_ACTIONS_H
#define PCB_HID_ACTIONS_H

#include "hid.h"

/* These are called from main_act.c */
void print_actions(void);
void dump_actions(void);


/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

const HID_Action *hid_find_action(const char *name);

extern void hid_remove_actions(const HID_Action * a, int n);
extern void hid_remove_action(const HID_Action * a);
extern void hid_remove_actions_by_cookie(const char *cookie);

int hid_action(const char *action_);
int hid_actionl(const char *action_, ...);	/* NULL terminated */
int hid_actionv(const char *action_, int argc_, const char **argv_);
int hid_actionv_(const HID_Action *a, int argc, const char **argv);

/* Parse the given command string into action calls, and call
   hid_actionv for each action found.  Accepts both "action(arg1,
   arg2)" and command-style "action arg1 arg2", allowing only one
   action in the later case.  Returns nonzero if the action handler(s)
   return nonzero. */
int hid_parse_command(const char *str_);

/* Parse the given string into action calls, and call
   hid_actionv for each action found.  Accepts only
   "action(arg1, arg2)" */
int hid_parse_actions(const char *str_);

#endif
