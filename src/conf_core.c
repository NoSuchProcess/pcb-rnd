#include "conf.h"
#include "conf_core.h"
conf_core_t conf_core;

void conf_core_init()
{
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc,flags);
#include "conf_core_fields.h"
}

#define conf_clamp_to(type, var, min, max, safe_val) \
do { \
	if ((var < min) || (var > max)) \
		*((type *)(&var)) = safe_val; \
}	while(0)

#define conf_clamp(type, var, min, max) \
do { \
	if (var < min) \
		*((type *)(&var)) = min; \
	else if (var > max) \
		*((type *)(&var)) = max; \
}	while(0)

static char *get_homedir(void)
{
	char *homedir = getenv("HOME");
	if (homedir == NULL)
		homedir = getenv("USERPROFILE");
	return homedir;
}

static void conf_ro(const char *path)
{
	conf_native_t *n = conf_get_field(path);
	if (n != NULL) {
		n->used = 1;
		n->random_flags.read_only = 1;
	}
}

void conf_core_postproc()
{
	conf_clamp_to(CFT_COORD, conf_core.design.line_thickness, MIN_LINESIZE, MAX_LINESIZE, PCB_MIL_TO_COORD(10));
	conf_clamp_to(CFT_COORD, conf_core.design.via_thickness, MIN_PINORVIASIZE, MAX_PINORVIASIZE, PCB_MIL_TO_COORD(40));
	conf_clamp_to(CFT_COORD, conf_core.design.via_drilling_hole, 0, MAX_COORD, DEFAULT_DRILLINGHOLE * conf_core.design.via_thickness / 100);
	conf_clamp(CFT_COORD, conf_core.design.max_width, MIN_SIZE, MAX_COORD);
	conf_clamp(CFT_COORD, conf_core.design.max_height, MIN_SIZE, MAX_COORD);
	conf_force_set_bool(conf_core.rc.have_regex, 1);
	conf_ro("rc/have_regex");

	conf_force_set_str(conf_core.rc.path.prefix, PCB_PREFIX);   conf_ro("rc/path/prefix");
	conf_force_set_str(conf_core.rc.path.lib, PCBLIBDIR);       conf_ro("rc/path/lib");
	conf_force_set_str(conf_core.rc.path.bin, BINDIR);          conf_ro("rc/path/bin");
	conf_force_set_str(conf_core.rc.path.share, PCBSHAREDIR);   conf_ro("rc/path/share");
	conf_force_set_str(conf_core.rc.path.home, get_homedir());  conf_ro("rc/path/home");
}
