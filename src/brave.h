#ifndef PCB_BRAVE_H
#define PCB_BRAVE_H
typedef enum { /* bitfield */
	PCB_BRAVE_OFF = 0,
	PCB_BRAVE_NOXOR = 1,
	PCB_BRAVE_NEWFIND = 2,
	PCB_BRACE_max
} pcb_brave_t;

extern pcb_brave_t pcb_brave; /* cache generated from the config */

void pcb_brave_init(void);
void pcb_brave_uninit(void);
#endif
