BEGIN {
	proc_args(P, "n,spacing", "n")

	set_arg(P, "?spacing", 300)

	P["n"] = int(P["n"])
	if ((P["n"] < 2) || ((P["n"] % 2) != 0))
		error("Number of pins have to be an even positive number")

	spacing=parse_dim(P["spacing"])

	element_begin(P["n"] "*" P["spacing"] " DIP socket", "U1", P["n"] "*" P["spacing"]    ,0,0, 0, mil(-100))

	half = mil(50)

	for(n = 1; n <= P["n"]/2; n++) {
		element_pin(0, (n-1) * mil(100), n)
		element_pin(spacing, (n-1) * mil(100), P["n"] - n + 1)
	}

	dip_outline(-half, -half, spacing + half , (n-2) * mil(100) + half,  half)

	element_end()
}
