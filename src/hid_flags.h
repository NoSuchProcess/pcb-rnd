#ifndef PCB_HID_FLAGS_H
#define  PCB_HID_FLAGS_H

/* Use this to temporarily enable all layers, so that they can be
   exported even if they're not currently visible.  save_array must be
   MAX_LAYER+2 big. */
void hid_save_and_show_layer_ons(int *save_array);
/* Use this to restore them.  */
void hid_restore_layer_ons(int *save_array);


/* Looks up one of the flags registered above.  If the flag is
   unknown, returns zero.  */
int hid_get_flag(const char *name_);

#endif
