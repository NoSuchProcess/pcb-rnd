BEGIN {
	
	set_arg(P, "?aspect", 6)
	set_arg(P, "?type", "normal")

	proc_args(P, "spacing,type,dia,aspect", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing/6)
	aspect = P["aspect"]

	offs_x = +spacing/2

	element_begin("acy" P["spacing"], "D1", "acy" P["spacing"]    ,0,0, 2.2*spacing/3,-mil(50))

	element_pin(-spacing/2, 0, 1)
	element_pin(+spacing/2, 0, 2)

# pins
	element_line(-spacing/2, 0, -spacing/aspect, 0)
	element_line(+spacing/aspect, 0, +spacing/2, 0)

# triangle
	element_line(+spacing/aspect, -dia, +spacing/aspect, +dia)
	element_line(+spacing/aspect, -dia, -spacing/aspect, 0)
	element_line(+spacing/aspect, +dia, -spacing/aspect, 0)

# front cross line with decoration
	r = dia*0.3
	if (P["type"] == "normal") {
		element_line(-spacing/aspect, -dia, -spacing/aspect, +dia)
	}
	else if (P["type"] == "zener") {
		element_line(-spacing/aspect, -dia, -spacing/aspect, +dia)
		element_line(-spacing/aspect, +dia, -spacing/aspect-r, +dia)
		element_line(-spacing/aspect, -dia, -spacing/aspect+r, -dia)
	}
	else if (P["type"] == "tunnel") {
		element_line(-spacing/aspect, -dia, -spacing/aspect, +dia)
		element_line(-spacing/aspect, +dia, -spacing/aspect+r, +dia)
		element_line(-spacing/aspect, -dia, -spacing/aspect+r, -dia)
	}
	else if (P["type"] == "varactor") {
		element_line(-spacing/aspect, -dia, -spacing/aspect, +dia)
		element_line(-spacing/aspect-r, -dia, -spacing/aspect-r, +dia)
	}
	else if (P["type"] == "schottky") {
		cx = -spacing/aspect + r
		cy = -(dia-r)
		element_line(-spacing/aspect, -(dia-r), -spacing/aspect, +dia-r)
		element_arc(cx, cy, r, r, 0, -180)
		cx = -spacing/aspect - r
		cy = +(dia-r)
		element_arc(cx, cy, r, r, 0, +180)
	}
	else if ((P["type"] != "") && (P["type"] != "none")) {
		error("Invalid type")
	}

	element_end()
}
