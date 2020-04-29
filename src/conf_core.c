#include "config.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/conf_hid.h>
#include <genvector/gds_char.h>
#include <genht/htpp.h>
#include <genht/hash.h>

TODO("win32: remove this once the w32_ dir workaround is removed")
#include <librnd/core/hid_init.h>

conf_core_t conf_core;

static  const char conf_core_cookie[] = "conf_core";
static htpp_t legacy_new2old, legacy_old2new;

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

static void pcb_conf_legacy_(rnd_conf_native_t *ndst, rnd_conf_native_t *nlegacy)
{
	gds_t tmp;
	const char *dst_path = ndst->hash_path;

	gds_init(&tmp);
	rnd_conf_print_native_field((conf_pfn)pcb_append_printf, &tmp, 0, &nlegacy->val, nlegacy->type, nlegacy->prop, 0);
	if (tmp.used > 0)
		rnd_conf_set(RND_CFR_INTERNAL, dst_path, -1, tmp.array, RND_POL_OVERWRITE);
	gds_uninit(&tmp);
}

void pcb_conf_legacy(const char *dst_path, const char *legacy_path)
{
	rnd_conf_native_t *nlegacy = rnd_conf_get_field(legacy_path);
	rnd_conf_native_t *ndst = rnd_conf_get_field(dst_path);
	if (nlegacy == NULL) {
		rnd_message(PCB_MSG_ERROR, "pcb_conf_legacy: invalid legacy path '%s' for %s\n", legacy_path, dst_path);
		return;
	}
	if (ndst == NULL) {
		rnd_message(PCB_MSG_ERROR, "pcb_conf_legacy: invalid new path %s\n", dst_path);
		return;
	}
	pcb_conf_legacy_(ndst, nlegacy);
	htpp_set(&legacy_new2old, ndst, nlegacy);
	htpp_set(&legacy_old2new, nlegacy, ndst);
}


static void conf_core_postproc(void)
{
	htpp_entry_t *e;

	conf_clamp_to(RND_CFT_COORD, conf_core.design.line_thickness, PCB_MIN_THICKNESS, PCB_MAX_THICKNESS, PCB_MIL_TO_COORD(10));
	conf_force_set_bool(conf_core.rc.have_regex, 1);
	rnd_conf_ro("rc/have_regex");

	conf_force_set_str(conf_core.rc.path.prefix, PCB_PREFIX);   rnd_conf_ro("rc/path/prefix");
	conf_force_set_str(conf_core.rc.path.lib, PCBLIBDIR);       rnd_conf_ro("rc/path/lib");
	conf_force_set_str(conf_core.rc.path.bin, BINDIR);          rnd_conf_ro("rc/path/bin");
	conf_force_set_str(conf_core.rc.path.share, PCBSHAREDIR);   rnd_conf_ro("rc/path/share");

	for(e = htpp_first(&legacy_new2old); e != NULL; e = htpp_next(&legacy_new2old, e)) {
		rnd_conf_native_t *nlegacy = e->value, *ndst = e->key;
		if (nlegacy->rnd_conf_rev > ndst->rnd_conf_rev)
			pcb_conf_legacy_(ndst, nlegacy);
	}
}

static void conf_legacy_chg(rnd_conf_native_t *ndst, int arr_idx)
{
 /* check if a legacy nde changes so we need to update a new node */
	rnd_conf_native_t *nlegacy;
	static int lock = 0;

	if (lock)
		return;

	nlegacy = htpp_get(&legacy_old2new, ndst);
	if (nlegacy == NULL)
		return;

	lock++;
	pcb_conf_legacy_(ndst, nlegacy); /* overwrite content on role INTERNAL */
	lock--;
}

static conf_hid_callbacks_t cbs;


void conf_core_uninit_pre(void)
{
	pcb_conf_hid_unreg(conf_core_cookie);
}

void conf_core_uninit(void)
{
	htpp_uninit(&legacy_new2old);
	htpp_uninit(&legacy_old2new);
}

void conf_core_init(void)
{
	htpp_init(&legacy_new2old, ptrhash, ptrkeyeq);
	htpp_init(&legacy_old2new, ptrhash, ptrkeyeq);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_core, field,isarray,type_name,cpath,cname,desc,flags);
#include "conf_core_fields.h"
	pcb_conf_core_postproc = conf_core_postproc;
	conf_core_postproc();

	cbs.val_change_post = conf_legacy_chg;
	pcb_conf_hid_reg(conf_core_cookie, &cbs);

	/* these old drc settings from editor/ are copied over to editor/drc
	   because various core and tool code depend on the values being at the
	   new place. */
	pcb_conf_legacy("design/drc/min_copper_clearance", "design/bloat");
	pcb_conf_legacy("design/drc/min_copper_overlap", "design/shrink");
}
