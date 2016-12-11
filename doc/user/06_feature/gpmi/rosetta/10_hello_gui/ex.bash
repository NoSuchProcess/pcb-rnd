#!/bin/bash

function load_packages()
{
	GPMI PkgLoad "pcb-rnd-gpmi/actions" 0
	GPMI PkgLoad "pcb-rnd-gpmi/dialogs" 0
}

function ev_action() {
	GPMI dialog_report "Greeting window" "Hello world!"
}

function main()
{
	load_packages
	GPMI Bind "ACTE_action"  "ev_action"
	GPMI action_register "hello" "" "log hello world" "hello()"
}
