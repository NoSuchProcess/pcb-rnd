/* 3-state plugin system; possible states of each plugin, stored in
   /local/pcb/PLUGIN_NAME/controls:
    "disable" = do not compile it at all
    "buildin" = enable, static link into the executable
    "plugin"  = enable, make it a dynamic link library (runtime load plugin)
*/

#define sdisable "disable"
#define sbuildin "buildin"
#define splugin  "plugin"


/* Macros to check the state */

#define plug_eq(name, val) \
	((get("/local/pcb/" name "/controls") != NULL) && (strcmp(get("/local/pcb/" name "/controls"), val) == 0))

#define plug_is_enabled(name)  (plug_eq(name, splugin) || plug_eq(name, sbuildin))
#define plug_is_disabled(name) (plug_eq(name, sdisabled))
#define plug_is_buildin(name)  (plug_eq(name, sbuildin))
#define plug_is_plugin(name)   (plug_eq(name, splugin))

/* auto-set tables to change control to the desired value */
const arg_auto_set_node_t arg_disable[] = {
	{"controls",    sdisable},
	{NULL, NULL}
};

const arg_auto_set_node_t arg_buildin[] = {
	{"controls",    sbuildin},
	{NULL, NULL}
};

const arg_auto_set_node_t arg_plugin[] = {
	{"controls",    splugin},
	{NULL, NULL}
};


/* plugin_def implementation to create CLI args */
#define plugin3_args(name, desc) \
	{"disable-" name, "/local/pcb/" name,  arg_disable,   "$do not compile " desc}, \
	{"buildin-" name, "/local/pcb/" name,  arg_buildin,   "$static link " desc " into the executable"}, \
	{"plugin-"  name, "/local/pcb/" name,  arg_plugin,    "$" desc " is a dynamic loadable plugin"},


/* plugin_def implementation to set default state */
#define plugin3_default(name, default_) \
	db_mkdir("/local/pcb/" name); \
	put("/local/pcb/" name "/controls", default_);

/* plugin_def implementation to print a report with the final state */
#define plugin3_stat(name, desc) \
	plugin_stat(desc, "/local/pcb/" name "/controls", name);
