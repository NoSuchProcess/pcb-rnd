#ifndef PCB_HID_FLAGS_H
#define  PCB_HID_FLAGS_H
/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

typedef struct {
	/* Name of the flag */
	char *name;
	/* Function to call to get the value of the flag.  */
	int (*function) (int);
	/* Additional parameter to pass to that function.  */
	int parm;
} HID_Flag;

HID_Flag *hid_find_flag(const char *name);

/* Use this to temporarily enable all layers, so that they can be
   exported even if they're not currently visible.  save_array must be
   MAX_LAYER+2 big. */
void hid_save_and_show_layer_ons(int *save_array);
/* Use this to restore them.  */
void hid_restore_layer_ons(int *save_array);


extern void hid_register_flags(HID_Flag *a, int n, const char *cookie, int copy);
#define REGISTER_FLAGS(a, cookie) HIDCONCAT(void register_,a) ()\
{ hid_register_flags(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* Remove all flags matching a cookie */
void hid_remove_flags_by_cookie(const char *cookie);

/* Looks up one of the flags registered above.  If the flag is
   unknown, returns zero.  */
int hid_get_flag(const char *name_);

/* Free all flags */
void hid_flags_uninit(void);

#endif
