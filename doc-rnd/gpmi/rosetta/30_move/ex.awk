BEGIN {
	PkgLoad("pcb-rnd-gpmi/actions", 0);
	PkgLoad("pcb-rnd-gpmi/layout", 0);
}

function ev_action(id, name, argc, x, y      ,dx,dy,n,num_objs,obj_ptr)
{
	dx = action_arg(0) * mm2pcb_multiplier()
	dy = action_arg(1) * mm2pcb_multiplier()

	num_objs = layout_search_selected("mv_search", "OM_ANY")
	for(n = 0; n < num_objs; n++) {
		obj_ptr = layout_search_get("mv_search", n)
		layout_obj_move(obj_ptr, "OC_OBJ", dx, dy)
	}
	layout_search_free("mv_search")
}

BEGIN {
	Bind("ACTE_action", "ev_action");
	action_register("mv", "", "move selected objects by dx and dy mm", "mv(dx,dy)");
}
