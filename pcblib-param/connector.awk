BEGIN {
	P["spacing"] = 100
	proc_args(P, "nx,ny,spacing", "nx,ny")

	step=P["spacing"]

	if (step ~ "mm") {
		sub("mm", "", step)
		step = mm(step)
	}
	else
		step = mil(step)

	half=step/2

	element_begin(spacing " mil connector", "CONN1", P["nx"] "*" P["ny"]    ,0,0, 0,mil(-100))

	for(x = 0; x < P["nx"]; x++)
		for(y = 0; y < P["ny"]; y++)
			element_pin(x * step, y * step)

	element_rectangle(-half, -half, P["nx"] * step-half, P["ny"] * step-half)

	element_line(0, -half,  -half, 0)

	element_end()
}
