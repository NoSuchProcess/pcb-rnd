extern pcb_tool_t pcb_tool_move;

void pcb_tool_move_uninit(void);
void pcb_tool_move_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_move_release_mode(pcb_hidlib_t *hl);
void pcb_tool_move_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_move_undo_act(pcb_hidlib_t *hl);
