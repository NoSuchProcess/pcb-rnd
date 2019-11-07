extern pcb_tool_t pcb_tool_poly;

void pcb_tool_poly_uninit(void);
void pcb_tool_poly_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_poly_adjust_attached_objects(pcb_hidlib_t *hl);
void pcb_tool_poly_draw_attached(pcb_hidlib_t *hl);
pcb_bool pcb_tool_poly_undo_act(pcb_hidlib_t *hl);
pcb_bool pcb_tool_poly_redo_act(pcb_hidlib_t *hl);
