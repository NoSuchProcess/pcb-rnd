extern pcb_tool_t pcb_tool_rectangle;

void pcb_tool_rectangle_uninit(void);
void pcb_tool_rectangle_notify_mode(void);
void pcb_tool_rectangle_adjust_attached_objects(void);
pcb_bool pcb_tool_rectangle_undo_act(void);
