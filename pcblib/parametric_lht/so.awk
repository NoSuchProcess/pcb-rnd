BEGIN {
	base_unit_mm = 0

	help_auto()
	set_arg(P, "?row_spacing", 250)
	set_arg(P, "?pad_spacing", 50)
	set_arg(P, "?ext_bloat", 10)
	set_arg(P, "?int_bloat", 55)

	proc_args(P, "n,row_spacing,pad_spacing,ext_bloat,int_bloat", "n")

	P["n"] = int(P["n"])
	if ((P["n"] < 2) || ((P["n"] % 2) != 0))
		error("Number of pins have to be an even positive number")

	row_spacing=parse_dim(P["row_spacing"])
	pad_spacing=parse_dim(P["pad_spacing"])
	ext_bloat=parse_dim(P["ext_bloat"])
	int_bloat=parse_dim(P["int_bloat"])

# translate origin to the middle (int() and -0.5 rounds it for odd number of pins)
	offs_x = -(row_spacing/2)
	offs_y = -int((P["n"]/4-0.5) * pad_spacing)

	subc_begin(P["n"] "*" P["row_spacing"], "U1", -offs_x, mil(-100)-offs_y)

	proto = subc_proto_create_pad_sqline(-ext_bloat, int_bloat, pad_width)

	for(n = 1; n <= P["n"]/2; n++) {
		y = (n-1) * pad_spacing
		subc_pstk(proto, 0, y, 0, n)
		subc_pstk(proto, row_spacing, y, 180, P["n"] - n + 1)
	}

	silk_dist_x = either(parse_dim(P["silk_ext_x"]), pad_spacing/2)
	silk_dist_y = either(parse_dim(P["silk_ext_y"]), pad_spacing/2)
	rarc = either(parse_dim(P["rarc"]), pad_spacing/2)

	dip_outline("top-silk", -silk_dist_x-ext_bloat, -silk_dist_y, row_spacing + silk_dist_x+ext_bloat , (n-2) * pad_spacing + silk_dist_y,  rarc)

	left_dim="@" -silk_dist_x-ext_bloat-pad_spacing ";0"
	dimension(0, 0, 0, pad_spacing, left_dim, "pad_spacing")
	dimension(0, 0, row_spacing, 0, (pad_spacing * 1.8), "row_spacing")
	dimension(-ext_bloat, 0, 0, 0, (pad_spacing * 1.2), "ext_bloat")
	dimension(row_spacing-int_bloat, 0, row_spacing, 0, (pad_spacing * 1.2), "int_bloat")

	th=DEFAULT["pad_thickness"]
	y = (n-2) * pad_spacing
	dimension(-ext_bloat-th/2, y, +int_bloat+th/2, y, (pad_spacing * -1.2), "<auto>")
	dimension(0, y-th/2, 0, y+th/2, left_dim, "pad_thickness")

#	dimension(-silk_dist_x-ext_bloat, -silk_dist_y, row_spacing + silk_dist_x+ext_bloat, -silk_dist_y, pad_spacing*2.5, "<auto>")

	subc_end()
}
