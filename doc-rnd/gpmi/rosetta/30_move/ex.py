PkgLoad("pcb-rnd-gpmi/actions", 0);
PkgLoad("pcb-rnd-gpmi/layout", 0);

def ev_action(id, name, argc, x, y):
	dx = int(action_arg(0)) * float(mm2pcb_multiplier())
	dy = int(action_arg(1)) * float(mm2pcb_multiplier())
	num_objs = int(layout_search_selected("mv_search", "OM_ANY"))
	for n in xrange(0, num_objs):
		obj_ptr = layout_search_get("mv_search", n)
		layout_obj_move(obj_ptr, "OC_OBJ", dx, dy)
	layout_search_free("mv_search")

Bind("ACTE_action", "ev_action");
action_register("hello", "", "log hello world", "hello()");
action_register("mv", "", "move selected objects by dx and dy mm", "mv(dx,dy)");
