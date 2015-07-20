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

	element_end()
}
