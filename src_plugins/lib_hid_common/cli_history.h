
/*** basic GUI integration ***/

/* Append a new command to the list after the user has entered/executed it */
typedef void pcb_clihist_append_cb_t(void *ctx, const char *cmd);
typedef void pcb_clihist_remove_cb_t(void *ctx, int idx);
void pcb_clihist_append(const char *cmd, void *ctx, pcb_clihist_append_cb_t *append, pcb_clihist_remove_cb_t *remove);


/* Initialize the command history, load the history from disk if needed */
void pcb_clihist_init(void);

/* Call a series of append's to get the GUI in sync with the list */
void pcb_clihist_sync(void *ctx, pcb_clihist_append_cb_t *append);


/*** Cursor operation ***/

/* Move the cursor 'up' (backward in history) and return the next entry's cmd */
const char *pcb_clihist_prev(void);

/* Move the cursor 'down' (forward in history) and return the next entry's cmd */
const char *pcb_clihist_next(void);

/* Reset the cursor (pointing to void, behind the most recent entry), while the
   user is typing a new entry */
void pcb_clihist_reset(void);


/*** Misc/internal ***/
/* Load the last saved list from disk */
void pcb_clihist_load(void);

/* Save the command history to disk - called automatically on uninit */
void pcb_clihist_save(void);

/* Free all memory used by the history */
void pcb_clihist_uninit(void);

