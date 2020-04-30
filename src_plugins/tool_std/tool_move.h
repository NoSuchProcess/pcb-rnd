extern rnd_tool_t pcb_tool_move;

void pcb_tool_move_uninit(void);
void pcb_tool_move_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_move_release_mode(rnd_hidlib_t *hl);
void pcb_tool_move_draw_attached(rnd_hidlib_t *hl);
rnd_bool pcb_tool_move_undo_act(rnd_hidlib_t *hl);
