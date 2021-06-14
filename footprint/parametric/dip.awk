BEGIN {
	base_unit_mm = 0

	help_auto()
	set_arg(P, "?spacing", 300)

	proc_args(P, "n,spacing", "n")

	P["n"] = int(P["n"])
	if ((P["n"] < 2) || ((P["n"] % 2) != 0))
		error("Number of pins have to be an even positive number")

	spacing=parse_dim(P["spacing"])

	subc_begin(P["n"] "*" P["spacing"], "U1", 0, mil(-100))

	half = mil(50)

	pstk_s = subc_proto_create_pin_square()
	pstk_r = subc_proto_create_pin_round()

	for(n = 1; n <= P["n"]/2; n++) {
		subc_pstk((n == 1 ? pstk_s : pstk_r), 0, (n-1) * mil(100), 0, n)
		subc_pstk(pstk_r, spacing, (n-1) * mil(100), 0, P["n"] - n + 1)
	}

	dip_outline("top-silk", -half, -half, spacing + half , (n-2) * mil(100) + half,  half)

	dimension(0, 0, spacing, 0, mil(100), "spacing")

	subc_end()
}
