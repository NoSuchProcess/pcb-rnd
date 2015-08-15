PkgLoad("pcb-rnd-gpmi/actions", 0);
PkgLoad("pcb-rnd-gpmi/dialogs", 0);

sub ev_action {
	my($id, $name, $argc, $x, $y) = @_;
	dialog_log("Hello world!\n");
}

Bind("ACTE_action", "ev_action");
action_register("hello", "", "log hello world", "hello()");
