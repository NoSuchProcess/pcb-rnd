PkgLoad("pcb-rnd-gpmi/actions", 0);
PkgLoad("pcb-rnd-gpmi/dialogs", 0);

sub ev_action {
	my($id, $name, $argc, $x, $y) = @_;
	dialog_report("Greeting window", "Hello world!");
}

sub ev_gui_init
{
	my($id, $name, $argc, $argv) = @_;
	create_menu("Plugins/GPMI scripting/hello", "hello()", "h", "Ctrl<Key>w", "tooltip for hello");
}

Bind("ACTE_action", "ev_action");
Bind("ACTE_gui_init", "ev_gui_init");
action_register("hello", "", "log hello world", "hello()");
