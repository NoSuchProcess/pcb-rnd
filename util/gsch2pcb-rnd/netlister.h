/* Run gnetlist to generate a netlist and a PCB board file.  gnetlist
 * has exit status of 0 even if it's given an invalid arg, so do some
 * stat() hoops to decide if gnetlist successfully generated the PCB
 * board file (only gnetlist >= 20030901 recognizes -m).
 */
int run_gnetlist(const char *pins_file, const char *net_file, const char *pcb_file, const char * basename, const gadl_list_t *largs);


