extern pcb_tool_t pcb_tool_arrow;

void pcb_tool_arrow_uninit(void);
void pcb_tool_arrow_notify_mode(pcb_hidlib_t *hl);
void pcb_tool_arrow_release_mode(pcb_hidlib_t *hl);
void pcb_tool_arrow_adjust_attached_objects(pcb_hidlib_t *hl);
