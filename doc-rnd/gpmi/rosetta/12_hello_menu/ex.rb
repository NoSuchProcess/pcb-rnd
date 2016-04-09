PkgLoad("pcb-rnd-gpmi/actions", 0);
PkgLoad("pcb-rnd-gpmi/dialogs", 0);

def ev_action(id, name, argc, x, y)
	dialog_report("Greeting window", "Hello world!");
end

def ev_gui_init(id, argc, argv)
	create_menu("/main_menu/Plugins/GPMI scripting/hello", "hello()", "h", "Ctrl<Key>w", "tooltip for hello");
end

Bind("ACTE_action", "ev_action");
Bind("ACTE_gui_init", "ev_gui_init");
action_register("hello", "", "log hello world", "hello()");
