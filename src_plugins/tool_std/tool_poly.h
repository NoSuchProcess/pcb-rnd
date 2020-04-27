extern pcb_tool_t pcb_tool_poly;

void pcb_tool_poly_uninit(void);
void pcb_tool_poly_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_poly_adjust_attached_objects(rnd_hidlib_t *hl);
void pcb_tool_poly_draw_attached(rnd_hidlib_t *hl);
pcb_bool pcb_tool_poly_undo_act(rnd_hidlib_t *hl);
pcb_bool pcb_tool_poly_redo_act(rnd_hidlib_t *hl);
