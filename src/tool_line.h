extern pcb_tool_t pcb_tool_line;

void pcb_tool_line_init(void);
void pcb_tool_line_uninit(void);
void pcb_tool_line_notify_mode(void);
void pcb_tool_line_adjust_attached_objects(void);
void pcb_tool_line_draw_attached(void);
pcb_bool pcb_tool_line_undo_act(void);
pcb_bool pcb_tool_line_redo_act(void);
