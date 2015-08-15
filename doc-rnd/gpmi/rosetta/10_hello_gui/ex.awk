BEGIN {
	PkgLoad("pcb-rnd-gpmi/actions", 0);
	PkgLoad("pcb-rnd-gpmi/dialogs", 0);
}

function ev_action(id, name, argc, x, y)
{
	dialog_report("Greeting window", "Hello world!");
}

BEGIN {
	Bind("ACTE_action", "ev_action");
	action_register("hello", "", "log hello world", "hello()");
}
