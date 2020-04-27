extern pcb_tool_t pcb_tool_buffer;

void pcb_tool_buffer_init(void);
void pcb_tool_buffer_uninit(void);
void pcb_tool_buffer_notify_mode(rnd_hidlib_t *hl);
void pcb_tool_buffer_release_mode(rnd_hidlib_t *hl);
void pcb_tool_buffer_draw_attached(rnd_hidlib_t *hl);
