extern rnd_tool_t pcb_tool_rectangle;

void pcb_tool_rectangle_uninit(void);
void pcb_tool_rectangle_notify_mode(rnd_design_t *hl);
void pcb_tool_rectangle_adjust_attached_objects(rnd_design_t *hl);
rnd_bool pcb_tool_rectangle_undo_act(rnd_design_t *hl);
