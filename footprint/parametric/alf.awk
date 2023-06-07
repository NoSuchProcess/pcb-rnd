BEGIN {
	base_unit_mm = 0

	help_auto()
	set_arg(P, "?aspect", 6)
	set_arg(P, "?type", "normal")

	proc_args(P, "spacing,type,dia,aspect", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing/6)
	aspect = P["aspect"]

	offs_x = +spacing/2

	body_dia = parse_dim(P["3dbodydia"])
	if (body_dia > 0)
		body_dia = ",body_dia=" rev_mm(body_dia);
	else
		body_dia = ""

	body_length = parse_dim(P["3dbodylength"])
	if (body_length > 0)
		body_length = ",body_length=" rev_mm(body_length);
	else
		body_length = ""

	SCAT["openscad"]="alf.scad"
	SCAT["openscad-param"]="pitch=" rev_mm(spacing) body_dia body_length

	subc_begin("acy" P["spacing"], "D1", 2.2*spacing/3-offs_x,-mil(50), 0, SCAT)

	proto_s = subc_proto_create_pin_square()
	proto_r = subc_proto_create_pin_round()

	subc_pstk(proto_s, -spacing/2, 0, 0, 1)
	subc_pstk(proto_r, +spacing/2, 0, 0, 2)

	dimension(-spacing/2, 0, +spacing/2, 0, dia*4, "spacing")

# pins
	subc_line("top-silk", -spacing/2, 0, -spacing/aspect, 0)
	subc_line("top-silk", +spacing/aspect, 0, +spacing/2, 0)

# triangle
	subc_line("top-silk", +spacing/aspect, -dia, +spacing/aspect, +dia)
	subc_line("top-silk", +spacing/aspect, -dia, -spacing/aspect, 0)
	subc_line("top-silk", +spacing/aspect, +dia, -spacing/aspect, 0)

	dimension(+spacing/aspect, -dia, +spacing/aspect, dia, "@" spacing*1.2 ";0", "dia")


# front cross line with decoration
	r = dia*0.3
	if (P["type"] == "normal") {
		subc_line("top-silk", -spacing/aspect, -dia, -spacing/aspect, +dia)
	}
	else if (P["type"] == "zener") {
		subc_line("top-silk", -spacing/aspect, -dia, -spacing/aspect, +dia)
		subc_line("top-silk", -spacing/aspect, +dia, -spacing/aspect-r, +dia)
		subc_line("top-silk", -spacing/aspect, -dia, -spacing/aspect+r, -dia)
	}
	else if (P["type"] == "tunnel") {
		subc_line("top-silk", -spacing/aspect, -dia, -spacing/aspect, +dia)
		subc_line("top-silk", -spacing/aspect, +dia, -spacing/aspect+r, +dia)
		subc_line("top-silk", -spacing/aspect, -dia, -spacing/aspect+r, -dia)
	}
	else if (P["type"] == "varactor") {
		subc_line("top-silk", -spacing/aspect, -dia, -spacing/aspect, +dia)
		subc_line("top-silk", -spacing/aspect-r, -dia, -spacing/aspect-r, +dia)
	}
	else if (P["type"] == "schottky") {
		cx = -spacing/aspect + r
		cy = -(dia-r)
		subc_line("top-silk", -spacing/aspect, -(dia-r), -spacing/aspect, +dia-r)
		subc_arc("top-silk", cx, cy, r, 0, -180)
		cx = -spacing/aspect - r
		cy = +(dia-r)
		subc_arc("top-silk", cx, cy, r, 0, +180)
	}
	else if ((P["type"] != "") && (P["type"] != "none")) {
		error("Invalid type")
	}

	subc_end()
}
