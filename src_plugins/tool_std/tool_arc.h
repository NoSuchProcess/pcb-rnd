extern pcb_tool_t pcb_tool_arc;

void pcb_tool_arc_init(void);
void pcb_tool_arc_uninit(void);
void pcb_tool_arc_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_arc_adjust_attached_objects(rnd_hidlib_t *hl);
void pcb_tool_arc_draw_attached(rnd_hidlib_t *hl);
rnd_bool pcb_tool_arc_undo_act(rnd_hidlib_t *hl);
