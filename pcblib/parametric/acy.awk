function wave(type, repeat,    step,x,y)
{
	step = len/repeat
	for(x = sx1; x < sx2; x += step) {
		if (type == 1) {
			element_arc(x+step/2, 0, step/2, step/2, 0, -180)
		}
		else if (type == 2) {
			y = dia
			element_line(x, 0, x+step/4, -y)
			element_line(x+step/4, -y, x+3*step/4, y)
			element_line(x+3*step/4, y, x+step, 0)
			
		}
	}
}

BEGIN {
	set_arg(P, "?type", "block")
	proc_args(P, "spacing,type,pol,dia", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing/6)

# oops, dia is a radius rather
	dia=dia/2

	offs_x = +spacing/2

	element_begin("acy" P["spacing"], "R1", "acy" P["spacing"]    ,0,0, spacing/2-spacing/5,-mil(20))

	element_pin(-spacing/2, 0, 1)
	element_pin(+spacing/2, 0, 2)

# silk pins
	if (P["type"] != "line") {
		element_line(-spacing/2, 0, -spacing/4, 0)
		element_line(+spacing/4, 0, +spacing/2, 0)
	}

# silk symbol
	sx1 = -spacing/4
	sx2 = +spacing/4
	len = sx2-sx1
	if (P["type"] == "block") {
		element_rectangle(sx1, -dia, sx2, +dia)
	}
	else if (P["type"] == "zigzag") {
		wave(2, 3)
	}
	else if (P["type"] == "coil") {
		wave(1, 4)
	}
	else if (P["type"] == "endcap") {
		cl1 = len/9
		cl2 = len/8
		y1 = dia*1.2
		y2 = dia
		rarc = dia/5
		element_line(sx1+cl2, y2, sx2-cl2, y2)
		element_line(sx1+cl2, y2, sx1+cl1, y1)
		element_line(sx2-cl2, y2, sx2-cl1, y1)
		
		element_line(sx1+cl2, -y2, sx2-cl2, -y2)
		element_line(sx1+cl2, -y2, sx1+cl1, -y1)
		element_line(sx2-cl2, -y2, sx2-cl1, -y1)

		element_rectangle(sx1, y1, sx1+cl1, -y1, "right,NE,SE", rarc)
		element_rectangle(sx2-cl1, y1, sx2, -y1, "left,NW,SW", rarc)
	}
	else if (P["type"] ~ "^core") {
		wave(1, 4)
		nlines = P["type"]
		sub("^core", "", nlines)
		if (nlines == "")
			nlines = 1

		cdist = 3 * DEFAULT["line_thickness"]
		y = -len/8
		for(nlines = int(nlines); nlines > 0; nlines--) {
			y-=cdist
			element_line(sx1, y, sx2, y)
		}
	}
	else if (P["type"] == "line") {
		element_line(-spacing/2, 0, +spacing/2, 0)
	}
	else if (P["type"] == "standing") {
		r = dia*2
		if (r < DEFAULT["pin_ringdia"]/2*1.2)
			r = DEFAULT["pin_ringdia"]/2*1.2
		element_arc(-spacing/2, 0, r, r, 0, 360)
		element_line(-spacing/2, 0, +spacing/2, 0)
	}
	else {
		error("Invalid type")
	}

# silk wiper
	if (P["wiper"] == "thermistor") {
		x = len/3
		element_line(-4*x/4, dia*2, -2*x/4, dia*2)
		element_line(-2*x/4, dia*2, +2*x/4, -dia*2)
	}
	else if (P["wiper"] == "aarrow") {
		x = len/3
		element_arrow(-2*x/4, dia*2, +2*x/4, -dia*2-mil(30))
	}
	else if (P["wiper"] == "parrow") {
		element_arrow(0, -dia*2-mil(30), 0, -dia)
	}
	else if (P["wiper"] == "looparrow") {
		y = -dia*2-mil(30)
		x = sx2+len/8
		element_arrow(0, y, 0, -dia)
		element_line(0, y, x, y)
		element_line(x, y, x, 0)
	}
	else if ((P["wiper"] != "none") && (P["wiper"] != "")) {
		error("Invalid wiper")
	}

# silk sign
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
	else if (P["pol"] == "dot") {
		r=2*DEFAULT["line_thickness"]/3
		element_arc(-spacing/4-r*3, -dia/2, r, r, 0, 360)
	}
	else if ((P["pol"] != "") && (P["pol"] != "none")) {
		error("Invalid pol")
	}

	element_end()
}
