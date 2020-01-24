extern pcb_tool_t pcb_tool_arc;

void pcb_tool_arc_init(void);
void pcb_tool_arc_uninit(void);
void pcb_tool_arc_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_arc_adjust_attached_objects(pcb_hidlib_t *hl);
void pcb_tool_arc_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_arc_undo_act(pcb_hidlib_t *hl);
