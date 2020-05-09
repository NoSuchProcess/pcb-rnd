extern const char *PCB_PTR_DOMAIN_LAYER;
extern const char *PCB_PTR_DOMAIN_LAYERGRP;
extern const char *PCB_PTR_DOMAIN_VIEWLIST;
extern const char *PCB_PTR_DOMAIN_FPMAP;


void pcb_actions_init_pcb_only(void);

/* Read and execute an action script from a file, batching redraws;
   return 0 if all actions returned 0 */
int pcb_act_execute_file(rnd_hidlib_t *hidlib, const char *fn);

