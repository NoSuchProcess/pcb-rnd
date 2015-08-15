#!/bin/bash

function load_packages()
{
	GPMI PkgLoad "pcb-rnd-gpmi/actions" 0
	GPMI PkgLoad "pcb-rnd-gpmi/layout" 0
}

function ev_action() {
	local dx dy n num_objs

	GPMI action_arg 0
	a0=`bash_int "$result"`

	GPMI action_arg 1
	a1=`bash_int "$result"`
	
	GPMI mm2pcb_multiplier
	conv=`bash_int "$result"`

	dx=$(($a0 * $conv))
	dy=$(($a1 * $conv))

	GPMI layout_search_selected "mv_search" "OM_ANY"
	num_objs=`bash_int "$result"`

	for n in `seq 0 $(($num_objs-1))`
	do
		GPMI layout_search_get "mv_search" $n
		obj_ptr="$result"
		GPMI layout_obj_move $obj_ptr "OC_OBJ" $dx $dy
	done
	GPMI layout_search_free "mv_search"
}

function main()
{
	load_packages
	GPMI Bind "ACTE_action"  "ev_action"
	GPMI action_register "mv" "" "move selected objects by dx and dy mm" "mv(dx,dy)"
}
