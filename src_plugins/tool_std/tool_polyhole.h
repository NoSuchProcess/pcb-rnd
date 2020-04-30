extern rnd_tool_t pcb_tool_polyhole;

void pcb_tool_polyhole_uninit(void);
void pcb_tool_polyhole_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_polyhole_adjust_attached_objects(rnd_hidlib_t *hl);
void pcb_tool_polyhole_draw_attached(rnd_hidlib_t *hl);
rnd_bool pcb_tool_polyhole_undo_act(rnd_hidlib_t *hl);
rnd_bool pcb_tool_polyhole_redo_act(rnd_hidlib_t *hl);
