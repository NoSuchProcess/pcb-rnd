BEGIN {
	
	proc_args(P, "spacing,pol,dia", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing/12)

	offs_x = +spacing/2

	element_begin("acy" P["spacing"], "R1", "acy" P["spacing"]    ,0,0, spacing/2-spacing/5,-mil(20))

	element_pin(-spacing/2, 0, 1)
	element_pin(+spacing/2, 0, 2)

	element_rectangle(-spacing/4, -dia, +spacing/4, +dia)
	element_line(-spacing/2, 0, -spacing/4, 0)
	element_line(+spacing/4, 0, +spacing/2, 0)

	if (P["pol"] == "sign") {
		size=mil(10)

		offs_y = size*2.2
		offs_x = DEFAULT["pin_ringdia"]/2+size*1.1
		element_line(-size, 0, +size, 0)

		offs_x = spacing - (DEFAULT["pin_ringdia"]/2+size*1.1)
		element_line(-size, 0, +size, 0)
		element_line(0, -size, 0, +size)
	}
	else if (P["pol"] == "bar") {
		offs=DEFAULT["line_thickness"]
		element_rectangle(-spacing/4-offs, -dia, -spacing/4+offs, +dia,  DEFAULT["line_thickness"]*4)
	}
	else if ((P["pol"] != "") && (P["pol"] != "none")) {
		error("Invalid pol")
	}

	element_end()
}
