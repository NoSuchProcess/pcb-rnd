extern pcb_tool_t pcb_tool_arc;

void pcb_tool_arc_init(void);
void pcb_tool_arc_uninit(void);
void pcb_tool_arc_notify_mode(void);
void pcb_tool_arc_adjust_attached_objects(void);
void pcb_tool_arc_draw_attached(void);
pcb_bool pcb_tool_arc_undo_act(void);
