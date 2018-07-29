#ifndef PCB_HID_CFG_INPUT_H
#define PCB_HID_CFG_INPUT_H

#include <liblihata/dom.h>
#include <genht/htip.h>
#include <genht/htpp.h>
#include "hid_cfg.h"

#define PCB_M_Mod0(n)  (1u<<(n))
typedef enum {
	PCB_M_Shift   = PCB_M_Mod0(0),
	PCB_M_Ctrl    = PCB_M_Mod0(1),
	PCB_M_Alt     = PCB_M_Mod0(2),
	PCB_M_Mod1    = PCB_M_Alt,
	/* PCB_M_Mod(3) is PCB_M_Mod0(4) */
	/* PCB_M_Mod(4) is PCB_M_Mod0(5) */
	PCB_M_Release = PCB_M_Mod0(6), /* there might be a random number of modkeys, but hopefully not this many */

	PCB_MB_LEFT   = PCB_M_Mod0(7),
	PCB_MB_MIDDLE = PCB_M_Mod0(8),
	PCB_MB_RIGHT  = PCB_M_Mod0(9),

/* scroll wheel */
	PCB_MB_SCROLL_UP     = PCB_M_Mod0(10),
	PCB_MB_SCROLL_DOWN   = PCB_M_Mod0(11),
	PCB_MB_SCROLL_LEFT   = PCB_M_Mod0(12),
	PCB_MB_SCROLL_RIGHT  = PCB_M_Mod0(13)
} pcb_hid_cfg_mod_t;
#undef PCB_M_Mod0

#define PCB_MB_ANY (PCB_MB_LEFT | PCB_MB_MIDDLE | PCB_MB_RIGHT | PCB_MB_SCROLL_UP | PCB_MB_SCROLL_DOWN | PCB_MB_SCROLL_LEFT | PCB_MB_SCROLL_RIGHT)
#define PCB_M_ANY  (PCB_M_Release-1)

/************************** MOUSE ***************************/

typedef struct {
	lht_node_t *mouse;
	htip_t *mouse_mask;
} pcb_hid_cfg_mouse_t;

int hid_cfg_mouse_init(pcb_hid_cfg_t *hr, pcb_hid_cfg_mouse_t *mouse);
void hid_cfg_mouse_action(pcb_hid_cfg_mouse_t *mouse, pcb_hid_cfg_mod_t button_and_mask, pcb_bool cmd_entry_active);


/************************** KEYBOARD ***************************/
#define HIDCFG_MAX_KEYSEQ_LEN 32
typedef struct hid_cfg_keyhash_s {
	unsigned short int mods;          /* of pcb_hid_cfg_mod_t */
	unsigned short int key_raw;
	unsigned short int key_tr;
} hid_cfg_keyhash_t;


typedef struct pcb_hid_cfg_keyseq_s  pcb_hid_cfg_keyseq_t;
struct pcb_hid_cfg_keyseq_s {
	hid_cfg_keyhash_t addr;

	unsigned long int keysym;  /* optional 32 bit long storage the GUI hid should cast to whatever the GUI backend supports */

	const lht_node_t *action_node; /* terminal node: end of sequence, run actions */

	htpp_t seq_next; /* ... or if node is NULL, a hash for each key that may follow the current one */
	pcb_hid_cfg_keyseq_t *parent;
};

/* Translate symbolic name to single-char keysym before processing; useful
   for shortcuts like "Return -> '\r'" which are otherwise hard to describe
   in text format */
typedef struct pcb_hid_cfg_keytrans_s {
		const char *name;
		char sym;
} pcb_hid_cfg_keytrans_t;

extern const pcb_hid_cfg_keytrans_t hid_cfg_key_default_trans[];

/* A complete tree of keyboard shortcuts/hotkeys */
typedef struct pcb_hid_cfg_keys_s {
	htpp_t keys;

	/* translate key sym description (the portion after <Key>) to key_char;
	   desc is a \0 terminated string, len is only a hint. Should return 0
	   on error. */
	unsigned short int (*translate_key)(const char *desc, int len);

	/* convert a key_char to human readable name, copy the string to out/out_len.
	   Return 0 on success. */
	int (*key_name)(unsigned short int key_char, char *out, int out_len);


	int auto_chr;                       /* if non-zero: don't call translate_key() for 1-char symbols, handle the default way */
	const pcb_hid_cfg_keytrans_t *auto_tr;  /* apply this table before calling translate_key() */

	/* current sequence state */
	pcb_hid_cfg_keyseq_t *seq[HIDCFG_MAX_KEYSEQ_LEN];
	int seq_len;
	int seq_len_action; /* when an action node is hit, save sequence length for executing the action while seq_len is reset */
} pcb_hid_cfg_keys_t;


/* Initialize a new keyboard context
   Returns 0 on success.
*/
int pcb_hid_cfg_keys_init(pcb_hid_cfg_keys_t *km);

/* Free km's fields recursively */
int pcb_hid_cfg_keys_uninit(pcb_hid_cfg_keys_t *km);

/* Add the next key of a key sequence; key sequences form a tree. A key starting
   a new key sequence should have parent set NULL, subsequent calls should have
   parent set to the previously returned keyseq value. Terminal is non-zero if
   this is the last key of the sequence.
   Raw vs. translated keys are desribed at pcb_hid_cfg_keys_input().
   Returns NULL on error and loads errmsg if not NULL */
pcb_hid_cfg_keyseq_t *pcb_hid_cfg_keys_add_under(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_keyseq_t *parent, pcb_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr, int terminal, const char **errmsg);

/* Add a new key using a description (read from a lihata file usually)
   If out_seq is not NULL, load the array with pointers pointing to each
   key in the sequence, up to out_seq_len.
   When key desc is a lihata node, it may be a list (multiple keys for the
   same action). In this case return value and seq are set using the first key.
   Returns -1 on failure or the length of the sequence.
*/
int pcb_hid_cfg_keys_add_by_desc(pcb_hid_cfg_keys_t *km, const lht_node_t *keydesc, const lht_node_t *action_node);
int pcb_hid_cfg_keys_add_by_desc_(pcb_hid_cfg_keys_t *km, const lht_node_t *keydesc, const lht_node_t *action_node, pcb_hid_cfg_keyseq_t **out_seq, int out_seq_len);
int pcb_hid_cfg_keys_add_by_strdesc(pcb_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node);
int pcb_hid_cfg_keys_add_by_strdesc_(pcb_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node, pcb_hid_cfg_keyseq_t **out_seq, int out_seq_len);


/* Allocate a new string and generate a human readable accel-text; mask determines
   which keys on the list are generated (when multiple key sequences are
   specified for the same action; from LSB to MSB, at most 32 keys)
   Caller needs to free the string; returns NULL on error.
   */
char *pcb_hid_cfg_keys_gen_accel(pcb_hid_cfg_keys_t *km, const lht_node_t *keydescn, unsigned long mask, const char *sep);

/* Process next input key stroke.
   Seq and seq_len must not be NULL as they are the internal state of multi-key
   processing. Load seq array with pointers pointing to each key in the
   sequence, up to seq_len.
   Key_raw is the raw key code for base keys, without any translation, key_tr
   is the translated character; e.g. shift+1 is always reported as key_raw=1
   and on US keyboard reported as key_tr=!
   Returns:
    -1 if the key stroke or sequence is invalid
     0 if more characters needed to complete the sequence
     + a positive integer means the lookup succeeded and the return value
       is the length of the resulting sequence.
*/
int pcb_hid_cfg_keys_input(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr);
int pcb_hid_cfg_keys_input_(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr, pcb_hid_cfg_keyseq_t **seq, int *seq_len);

/* Run the action for a key sequence looked up by pcb_hid_cfg_keys_input().
   Returns: the result of the action or -1 on error */
int pcb_hid_cfg_keys_action(pcb_hid_cfg_keys_t *km);
int pcb_hid_cfg_keys_action_(pcb_hid_cfg_keyseq_t **seq, int seq_len);

/* Print a squence into dst in human readable form; returns strlen(dst) */
int pcb_hid_cfg_keys_seq(pcb_hid_cfg_keys_t *km, char *dst, int dst_len);
int pcb_hid_cfg_keys_seq_(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_keyseq_t **seq, int seq_len, char *dst, int dst_len);

#endif
