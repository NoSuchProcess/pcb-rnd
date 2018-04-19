extern pcb_tool_t pcb_tool_insert;

void pcb_tool_insert_uninit(void);
void pcb_tool_insert_notify_mode(void);
void pcb_tool_insert_adjust_attached_objects(void);
void pcb_tool_insert_draw_attached(void);
pcb_bool pcb_tool_insert_undo_act(void);
