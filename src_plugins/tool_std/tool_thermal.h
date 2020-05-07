extern rnd_tool_t pcb_tool_thermal;

void pcb_tool_thermal_notify_mode(rnd_hidlib_t *hl);

/* direct call: cycle through thermal modes on a padstack - provided for tool_via.h */
void pcb_tool_thermal_on_obj(pcb_any_obj_t *ps, unsigned long lid);
