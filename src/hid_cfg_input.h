#ifndef PCB_HID_CFG_INPUT_H
#define PCB_HID_CFG_INPUT_H

#include <liblihata/dom.h>
#include <genht/htip.h>
#include "hid_cfg.h"

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

/************************** MOUSE ***************************/

typedef struct {
	lht_node_t *mouse;
	htip_t *mouse_mask;
} hid_cfg_mouse_t;

int hid_cfg_mouse_init(hid_cfg_t *hr, hid_cfg_mouse_t *mouse);
void hid_cfg_mouse_action(hid_cfg_mouse_t *mouse, hid_cfg_mod_t button_and_mask);


/************************** KEYBOARD ***************************/
#define HIDCFG_MAX_KEYSEQ_LEN 32
typedef union hid_cfg_keyhash_u { /* integer hash key */
	unsigned long hash;
	struct {
		unsigned short int mods;          /* of hid_cfg_mod_t */
		unsigned short int key_char;
	} details;
} hid_cfg_keyhash_t;


typedef struct hid_cfg_keyseq_s  hid_cfg_keyseq_t;
struct hid_cfg_keyseq_s {
	unsigned long int keysym;  /* optional 32 bit long storage the GUI hid should cast to whatever the GUI backend supports */

	lht_node_t *action_node; /* terminal node: end of sequence, run actions */

	htip_t seq_next; /* ... or if node is NULL, a hash for each key that may follow the current one */
	hid_cfg_keyseq_t *parent;
};

/* Translate symbolic name to single-char keysym before processing; useful
   for shortcuts like "Return -> '\r'" which are otherwise hard to describe
   in text format */
typedef struct hid_cfg_keytrans_s {
		char *name;
		char sym;
} hid_cfg_keytrans_t;

extern const hid_cfg_keytrans_t hid_cfg_key_default_trans[];

/* A complete tree of keyboard shortcuts/hotkeys */
typedef struct hid_cfg_keys_s {
	htip_t keys;

	/* translate key sym description (the portion after <Key>) to key_char;
	   desc is a \0 terminated string, len is only a hint. Should return 0
	   on error. */
	unsigned short int (*translate_key)(char *desc, int len);

	int auto_chr;                       /* if non-zero: don't call translate_key() for 1-char symbols, handle the default way */
	const hid_cfg_keytrans_t *auto_tr;  /* apply this table before calling translate_key() */
} hid_cfg_keys_t;


/* Initialize a new keyboard context 
   Returns 0 on success.
*/
int hid_cfg_keys_init(hid_cfg_keys_t *km);

/* Free km's fields recursively */
int hid_cfg_keys_uninit(hid_cfg_keys_t *km);

/* Add the next key of a key sequence; key sequences form a tree. A key starting
   a new key sequence should have parent set NULL, subsequent calls should have
   parent set to the previously returned keyseq value. Terminal is non-zero if
   this is the last key of the sequence.
   Returns NULL on error */
hid_cfg_keyseq_t *hid_cfg_keys_add_under(hid_cfg_keys_t *km, hid_cfg_keyseq_t *parent, hid_cfg_mod_t mods, unsigned short int key_char, int terminal);

/* Add a new key using a description (read from a lihata file usually)
   If out_seq is not NULL, load the array with pointers pointing to each
   key in the sequence, up to out_seq_len.
   Returns -1 on failure or the length of the sequence.
*/
int hid_cfg_keys_add_by_desc(hid_cfg_keys_t *km, const char *keydesc, lht_node_t *action_node, hid_cfg_keyseq_t **out_seq, int out_seq_len);

/* Process next input key stroke.
   Seq and seq_len must not be NULL as they are the internal state of multi-key
   processing. Load seq array with pointers pointing to each key in the
   sequence, up to seq_len.
   Returns:
    -1 if the key stroke or sequence is invalid
     0 if more characters needed to complete the sequence
     + a positive integer means the lookup succeeded and the return value
       is the length of the resulting sequence.
*/
int hid_cfg_keys_input(hid_cfg_keys_t *km, hid_cfg_mod_t mods, unsigned short int key_char, hid_cfg_keyseq_t **seq, int *seq_len);

/* Run the action for a key sequence looked up by hid_cfg_keys_input().
   Returns: the result of the action or -1 on error */
int hid_cfg_keys_action(hid_cfg_keyseq_t **seq, int seq_len);


#endif
