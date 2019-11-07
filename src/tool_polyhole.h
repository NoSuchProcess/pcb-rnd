extern pcb_tool_t pcb_tool_polyhole;

void pcb_tool_polyhole_uninit(void);
void pcb_tool_polyhole_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_polyhole_adjust_attached_objects(pcb_hidlib_t *hl);
void pcb_tool_polyhole_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_polyhole_undo_act(pcb_hidlib_t *hl);
pcb_bool pcb_tool_polyhole_redo_act(pcb_hidlib_t *hl);
