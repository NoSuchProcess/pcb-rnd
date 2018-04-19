extern pcb_tool_t pcb_tool_move;

void pcb_tool_move_uninit(void);
void pcb_tool_move_notify_mode(void);
void pcb_tool_move_release_mode (void);
void pcb_tool_move_draw_attached(void);
pcb_bool pcb_tool_move_undo_act(void);
