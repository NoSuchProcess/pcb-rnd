#!/bin/bash

function load_packages()
{
	GPMI PkgLoad "pcb-rnd-gpmi/actions" 0
	GPMI PkgLoad "pcb-rnd-gpmi/dialogs" 0
}

function ev_action() {
	GPMI dialog_report "Greeting window" "Hello world!"
}

function ev_gui_init()
{
	GPMI create_menu "/main_menu/Plugins/GPMI scripting/hello" "hello()" "h" "Ctrl<Key>w" "tooltip for hello"
}

function main()
{
	load_packages
	GPMI Bind "ACTE_action"  "ev_action"
	GPMI Bind "ACTE_gui_init" "ev_gui_init"
	GPMI action_register "hello" "" "log hello world" "hello()"
}
