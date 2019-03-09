#ifndef PCB_BRAVE_H
#define PCB_BRAVE_H
typedef enum { /* bitfield */
	PCB_BRAVE_OFF = 0,
	PCB_BRAVE_NOXOR = 1,
	PCB_BRAVE_CLIPBATCH = 2,
	PCB_BRAVE_LESSTIF_TREETABLE = 4,
	PCB_BRAVE_LESSTIF_NEWTABBED = 8,
	PCB_BRAVE_max
} pcb_brave_t;

extern pcb_brave_t pcb_brave; /* cache generated from the config */

void pcb_brave_init(void);
void pcb_brave_uninit(void);
#endif
