BEGIN {
	P["spacing"] = 100
	P["silkmark"] = "square"

	proc_args(P, "nx,ny,spacing", "nx,ny")

	step=P["spacing"]

	if (step ~ "mm") {
		sub("mm", "", step)
		step = mm(step)
	}
	else
		step = mil(step)

	half=step/2

	element_begin(spacing " mil connector", "CONN1", P["nx"] "*" P["ny"]    ,0,0, 0, -step)

	for(x = 0; x < P["nx"]; x++)
		for(y = 0; y < P["ny"]; y++)
			element_pin(x * step, y * step)

	element_rectangle(-half, -half, P["nx"] * step-half, P["ny"] * step-half)

	if (P["silkmark"] == "angled") {
		element_line(0, -half,  -half, 0)
	}
	else if (P["silkmark"] == "square") {
		element_line(-half, half,  half, half)
		element_line(half, -half,  half, half)
	}
	else if (P["silkmark"] == "external") {
		element_line(-half, 0,        -step, -half/2)
		element_line(-half, 0,        -step, +half/2)
		element_line(-step, -half/2,  -step, +half/2)
	}

	element_end()
}
