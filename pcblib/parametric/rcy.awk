BEGIN {
	
	set_arg(P, "?pol", "sign")
	proc_args(P, "spacing,pol,dia", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing*2)

	offs_x = +spacing/2

	element_begin("acy" P["spacing"], "R1", "acy" P["spacing"]    ,0,0, spacing/2-spacing/5,-mil(20))

	element_pin(-spacing/2, 0, 1)
	element_pin(+spacing/2, 0, 2)

# silk rectangle and pins
	element_arc(0, 0, dia/2, dia/2, 0, 360)

	if (P["pol"] == "sign") {
		size=mil(20)

		offs_x = spacing/2 +dia/2+size*2
		element_line(-size, 0, +size, 0)

		offs_x = spacing/2 -dia/2-size*2
		element_line(-size, 0, +size, 0)
		element_line(0, -size, 0, +size)
	}
	else if (P["pol"] ~ "^bar") {
# determine bar side (default to -)
		side=P["pol"]
		sub("^bar", "", side)
		if (side == "")
			side = "-"
		side = int(side "1") * -1

		th = mm(1)
		ystep = th/2
		r = dia/2-th/2
		xs = dia/8
		ring = DEFAULT["pin_ringdia"]
		for(y = 0; y < dia/2; y+=ystep) {
			x = r*r-y*y
			if (x < 0)
				break
			x = sqrt(x)
			if (x <= xs)
				break
			if (y > ring/2+th/2) {
				element_line(side*xs, y, side*x, y, th)
				if (y != 0)
					element_line(side*xs, -y, side*x, -y, th)
			}
			else {
# keep out a rectangle around the pin
				end1=spacing/2-ring
				end2=spacing/2+ring
				if (end1 > xs)
					element_line(side*xs, y, side*end1, y, th)
				if (end2 < x)
					element_line(side*end2, y, side*x, y, th)
				if (y != 0) {
					if (end1 > xs)
						element_line(side*xs, -y, side*end1, -y, th)
					if (end2 < x)
						element_line(side*end2, -y, side*x, -y, th)
				}
			}
		}

	}
	else if ((P["pol"] != "") && (P["pol"] != "none")) {
		error("Invalid pol")
	}

	element_end()
}
