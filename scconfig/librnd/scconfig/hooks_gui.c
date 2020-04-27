/* if any of these are enabled, we need the dialog plugin; dialog can not be
   a pup dep because it must be omitted from the hidlib */
static const char *dialog_deps[] = {
	"/local/pcb/dialogs/controls",         /* so we don't relax user's explicit request */
	"/local/pcb/hid_remote/controls",
	"/local/pcb/lib_gtk_common/controls",
	"/local/pcb/hid_lesstif/controls",
	NULL
};


void rnd_calc_dialog_deps(void)
{
	const char **p;
	int buildin = 0, plugin = 0;

	for(p = dialog_deps; *p != NULL; p++) {
		const char *st = get(*p);
		if (strcmp(st, "buildin") == 0) {
			buildin = 1;
			break;
		}
		if (strcmp(st, "plugin") == 0)
			plugin = 1;
	}

	put("/target/librnd/dialogs/buildin", buildin ? strue : sfalse);
	put("/target/librnd/dialogs/plugin",  plugin  ? strue : sfalse);
}
