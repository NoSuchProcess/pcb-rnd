#ifndef RND_HID_COMMON_HIDNOGUI_H
#define RND_HID_COMMON_HIDNOGUI_H

void rnd_hid_nogui_init(rnd_hid_t * hid);
rnd_hid_t *rnd_hid_nogui_get_hid(void);

/* For checking if attr dialogs are not available: */
void *rnd_nogui_attr_dlg_new(rnd_hid_t *hid, const char *id, rnd_hid_attribute_t *attrs_, int n_attrs_, const char *title_, void *caller_data, rnd_bool modal, void (*button_cb)(void *caller_data, rnd_hid_attr_ev_t ev), int defx, int defy, int minx, int miny);

int rnd_nogui_progress(long so_far, long total, const char *message);

#endif
