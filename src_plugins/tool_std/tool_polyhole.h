extern rnd_tool_t pcb_tool_polyhole;

void pcb_tool_polyhole_uninit(void);
void pcb_tool_polyhole_notify_mode(rnd_design_t *hl);
void pcb_tool_polyhole_adjust_attached_objects(rnd_design_t *hl);
void pcb_tool_polyhole_draw_attached(rnd_design_t *hl);
rnd_bool pcb_tool_polyhole_undo_act(rnd_design_t *hl);
rnd_bool pcb_tool_polyhole_redo_act(rnd_design_t *hl);
