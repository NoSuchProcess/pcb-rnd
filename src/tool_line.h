extern pcb_tool_t pcb_tool_line;

void pcb_tool_line_init(void);
void pcb_tool_line_uninit(void);
void pcb_tool_line_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_line_adjust_attached_objects(pcb_hidlib_t *hl);
void pcb_tool_line_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_line_undo_act(pcb_hidlib_t *hl);
pcb_bool pcb_tool_line_redo_act(pcb_hidlib_t *hl);
