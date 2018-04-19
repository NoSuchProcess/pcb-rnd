extern pcb_tool_t pcb_tool_copy;

void pcb_tool_copy_uninit(void);
void pcb_tool_copy_notify_mode(void);
void pcb_tool_copy_release_mode (void);
void pcb_tool_copy_draw_attached(void);
pcb_bool pcb_tool_copy_undo_act(void);
