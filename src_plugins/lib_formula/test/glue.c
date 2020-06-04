/* glue for hidlib */

#include <librnd/core/hidlib.h>

const char *rnd_menu_file_paths[4];
const char *rnd_menu_name_fmt = "pcb-menu-%s.lht";

const char *rnd_hidlib_default_embedded_menu = "";

void rnd_hidlib_crosshair_move_to(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot) { }

const char *rnd_conf_userdir_path;
const char *rnd_pcphl_conf_user_path;
const char *rnd_conf_sysdir_path;
const char *rnd_conf_sys_path;

const char pcb_conf_internal_arr[] = {""}, *pcb_conf_internal = pcb_conf_internal_arr;
