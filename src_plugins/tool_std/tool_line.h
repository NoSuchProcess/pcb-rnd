extern pcb_tool_t pcb_tool_line;

void pcb_tool_line_init(void);
void pcb_tool_line_uninit(void);
void pcb_tool_line_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_line_adjust_attached_objects(rnd_hidlib_t *hl);
void pcb_tool_line_draw_attached(rnd_hidlib_t *hl);
rnd_bool pcb_tool_line_undo_act(rnd_hidlib_t *hl);
rnd_bool pcb_tool_line_redo_act(rnd_hidlib_t *hl);
