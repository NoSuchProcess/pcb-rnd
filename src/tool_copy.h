extern pcb_tool_t pcb_tool_copy;

void pcb_tool_copy_uninit(void);
void pcb_tool_copy_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_copy_release_mode(pcb_hidlib_t *hl);
void pcb_tool_copy_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_copy_undo_act(pcb_hidlib_t *hl);
