extern pcb_tool_t pcb_tool_polyhole;

void pcb_tool_polyhole_uninit(void);
void pcb_tool_polyhole_notify_mode(void);
void pcb_tool_polyhole_adjust_attached_objects(void);
void pcb_tool_polyhole_draw_attached(void);
pcb_bool pcb_tool_polyhole_undo_act(void);
pcb_bool pcb_tool_polyhole_redo_act(void);
