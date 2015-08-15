BEGIN {
	PkgLoad("pcb-rnd-gpmi/actions", 0);
	PkgLoad("pcb-rnd-gpmi/dialogs", 0);
}

function ev_action(id, name, argc, x, y)
{
	dialog_log("Hello world!\n");
}

BEGIN {
	Bind("ACTE_action", "ev_action");
	action_register("hello", "", "log hello world", "hello()");
}
