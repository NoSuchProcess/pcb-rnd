extern rnd_tool_t pcb_tool_buffer;
extern rnd_tool_t pcb_tool_pastebuffer;

void pcb_tool_buffer_init(void);
void pcb_tool_buffer_uninit(void);
void pcb_tool_buffer_notify_mode(rnd_design_t *hl);
void pcb_tool_buffer_release_mode(rnd_design_t *hl);
void pcb_tool_buffer_draw_attached(rnd_design_t *hl);
