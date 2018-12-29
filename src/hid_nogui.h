#ifndef PCB_HID_COMMON_HIDNOGUI_H
#define PCB_HID_COMMON_HIDNOGUI_H

void pcb_hid_nogui_init(pcb_hid_t * hid);
pcb_hid_t *pcb_hid_nogui_get_hid(void);

/* For checking if attr dialogs are not available: */
void *pcb_nogui_attr_dlg_new(const char *id, pcb_hid_attribute_t *attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy);

int pcb_nogui_progress(long so_far, long total, const char *message);

#endif
