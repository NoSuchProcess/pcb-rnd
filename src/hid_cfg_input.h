#ifndef PCB_HID_CFG_INPUT_H
#define PCB_HID_CFG_INPUT_H

#include <liblihata/dom.h>
#include <genht/htip.h>

typedef struct {
	lht_node_t *mouse;
	htip_t *mouse_mask;
} hid_cfg_mouse_t;

/* For compatibility; TODO: REMOVE */
#define M_Mod(n)  (1u<<(n+1))

#define M_Mod0(n)  (1u<<(n))
typedef enum {
	M_Shift   = M_Mod0(0),
	M_Ctrl    = M_Mod0(1),
	M_Alt     = M_Mod0(2),
	M_Multi   = M_Mod0(3), /* not a key; indicates that the keystroke is part of a multi-stroke sequence */
	/* M_Mod(3) is M_Mod0(4) */
	/* M_Mod(4) is M_Mod0(5) */
	M_Release = M_Mod0(6), /* there might be a random number of modkeys, but hopefully not this many */

	MB_LEFT   = M_Mod0(7),
	MB_MIDDLE = M_Mod0(8),
	MB_RIGHT  = M_Mod0(9),
	MB_UP     = M_Mod0(10), /* scroll wheel */
	MB_DOWN   = M_Mod0(11)  /* scroll wheel */
} hid_cfg_mod_t;

#define MB_ANY (MB_LEFT | MB_MIDDLE | MB_RIGHT | MB_UP | MB_DOWN)
#define M_ANY  (M_Release-1)

int hid_cfg_mouse_init(hid_cfg_t *hr, hid_cfg_mouse_t *mouse);
void hid_cfg_mouse_action(hid_cfg_mouse_t *mouse, hid_cfg_mod_t button_and_mask);

#endif
