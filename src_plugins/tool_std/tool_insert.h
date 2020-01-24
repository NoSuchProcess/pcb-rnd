extern pcb_tool_t pcb_tool_insert;

void pcb_tool_insert_uninit(void);
void pcb_tool_insert_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_insert_adjust_attached_objects(pcb_hidlib_t *hl);
void pcb_tool_insert_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_insert_undo_act(pcb_hidlib_t *hl);
