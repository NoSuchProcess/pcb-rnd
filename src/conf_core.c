#include "config.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <genvector/gds_char.h>
#include <genht/htpp.h>
#include <genht/hash.h>

TODO("win32: remove this once the w32_ dir workaround is removed")
#include <librnd/core/hid_init.h>

conf_core_t conf_core;

static htpp_t legacy_hash;

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

static void pcb_conf_legacy_(conf_native_t *ndst, conf_native_t *nlegacy)
{
	gds_t tmp;
	const char *dst_path = ndst->hash_path;

	gds_init(&tmp);
	pcb_conf_print_native_field((conf_pfn)pcb_append_printf, &tmp, 0, &nlegacy->val, nlegacy->type, nlegacy->prop, 0);
	if (tmp.used > 0)
		pcb_conf_set(CFR_INTERNAL, dst_path, -1, tmp.array, POL_OVERWRITE);
	gds_uninit(&tmp);
}

void pcb_conf_legacy(const char *dst_path, const char *legacy_path)
{
	conf_native_t *nlegacy = pcb_conf_get_field(legacy_path);
	conf_native_t *ndst = pcb_conf_get_field(dst_path);
	if (nlegacy == NULL) {
		pcb_message(PCB_MSG_ERROR, "pcb_conf_legacy: invalid legacy path '%s' for %s\n", legacy_path, dst_path);
		return;
	}
	if (ndst == NULL) {
		pcb_message(PCB_MSG_ERROR, "pcb_conf_legacy: invalid new path %s\n", dst_path);
		return;
	}
	pcb_conf_legacy_(ndst, nlegacy);
	htpp_set(&legacy_hash, ndst, nlegacy);
}


static void conf_core_postproc(void)
{
	htpp_entry_t *e;

	conf_clamp_to(CFT_COORD, conf_core.design.line_thickness, PCB_MIN_THICKNESS, PCB_MAX_THICKNESS, PCB_MIL_TO_COORD(10));
	conf_force_set_bool(conf_core.rc.have_regex, 1);
	pcb_conf_ro("rc/have_regex");

	conf_force_set_str(conf_core.rc.path.prefix, PCB_PREFIX);   pcb_conf_ro("rc/path/prefix");
	conf_force_set_str(conf_core.rc.path.lib, PCBLIBDIR);       pcb_conf_ro("rc/path/lib");
	conf_force_set_str(conf_core.rc.path.bin, BINDIR);          pcb_conf_ro("rc/path/bin");
	conf_force_set_str(conf_core.rc.path.share, PCBSHAREDIR);   pcb_conf_ro("rc/path/share");

	for(e = htpp_first(&legacy_hash); e != NULL; e = htpp_next(&legacy_hash, e)) {
		conf_native_t *nlegacy = e->value, *ndst = e->key;
		if (nlegacy->pcb_conf_rev > ndst->pcb_conf_rev)
			pcb_conf_legacy_(ndst, nlegacy);
	}
}

void conf_core_uninit(void)
{
	htpp_uninit(&legacy_hash);
}

void conf_core_init(void)
{
	htpp_init(&legacy_hash, ptrhash, ptrkeyeq);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc,flags);
#include "conf_core_fields.h"
	pcb_conf_core_postproc = conf_core_postproc;
	conf_core_postproc();

	/* these old drc settings from editor/ are copied over to editor/drc
	   because various core and tool code depend on the values being at the
	   new place. */
	pcb_conf_legacy("design/drc/min_copper_clearance", "design/bloat");
	pcb_conf_legacy("design/drc/min_copper_overlap", "design/shrink");
}
