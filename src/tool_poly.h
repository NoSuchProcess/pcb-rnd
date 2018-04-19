extern pcb_tool_t pcb_tool_poly;

void pcb_tool_poly_uninit(void);
void pcb_tool_poly_notify_mode(void);
void pcb_tool_poly_adjust_attached_objects(void);
void pcb_tool_poly_draw_attached(void);
pcb_bool pcb_tool_poly_undo_act(void);
pcb_bool pcb_tool_poly_redo_act(void);
