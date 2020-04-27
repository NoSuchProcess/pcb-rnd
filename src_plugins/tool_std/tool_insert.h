extern pcb_tool_t pcb_tool_insert;

void pcb_tool_insert_uninit(void);
void pcb_tool_insert_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_insert_adjust_attached_objects(rnd_hidlib_t *hl);
void pcb_tool_insert_draw_attached(rnd_hidlib_t *hl);
rnd_bool pcb_tool_insert_undo_act(rnd_hidlib_t *hl);
