PkgLoad("pcb-rnd-gpmi/actions", 0);
PkgLoad("pcb-rnd-gpmi/layout", 0);

def ev_action(id, name, argc, x, y)
# TODO: how to convert 1e+06 in ruby?
#	conv = mm2pcb_multiplier().to_i
	conv = 1000000


	dx = action_arg(0).to_i * conv.to_i;
	dy = action_arg(1).to_i * conv.to_i;

	num_objs = layout_search_selected("mv_search", "OM_ANY").to_i;

	for n in 0..(num_objs-1)
		obj_ptr = layout_search_get("mv_search", n);
		layout_obj_move(obj_ptr, "OC_OBJ", dx, dy);
		n += 1;
	end
	layout_search_free("mv_search");

end

Bind("ACTE_action", "ev_action");
action_register("mv", "", "move selected objects by dx and dy mm", "mv(dx,dy)");
