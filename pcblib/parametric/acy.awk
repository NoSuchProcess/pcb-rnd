function wave(type, repeat,    step,x,y)
{
	step = len/repeat
	for(x = sx1; x < sx2; x += step) {
		if (type == 1) {
			subc_arc("top-silk", x+step/2, 0, step/2, 0, -180)
		}
		else if (type == 2) {
			y = dia
			subc_line("top-silk", x, 0, x+step/4, -y)
			subc_line("top-silk", x+step/4, -y, x+3*step/4, y)
			subc_line("top-silk", x+3*step/4, y, x+step, 0)
			
		}
	}
}

BEGIN {
	base_unit_mm = 0

	help_auto()
	set_arg(P, "?type", "block")
	proc_args(P, "spacing,type,pol,dia", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing/6)

# oops, dia is a radius rather
	dia=dia/2

	offs_x = +spacing/2

	subc_begin("acy" P["spacing"], "R1", -spacing/5, -mil(20), 0)

	proto_s = subc_proto_create_pin_square()
	proto_r = subc_proto_create_pin_round()

	subc_pstk(proto_s, -spacing/2, 0, 0, 1)
	subc_pstk(proto_r, +spacing/2, 0, 0, 2)

	dimension(-spacing/2, 0, +spacing/2, 0, dia*4, "spacing")

# silk pins
	if (P["type"] != "line") {
		subc_line("top-silk", -spacing/2, 0, -spacing/4, 0)
		subc_line("top-silk", +spacing/4, 0, +spacing/2, 0)
	}

# silk symbol
	sx1 = -spacing/4
	sx2 = +spacing/4
	len = sx2-sx1
	if (P["type"] == "block") {
		subc_rectangle("top-silk", sx1, -dia, sx2, +dia)
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
		subc_line("top-silk", sx1+cl2, y2, sx2-cl2, y2)
		subc_line("top-silk", sx1+cl2, y2, sx1+cl1, y1)
		subc_line("top-silk", sx2-cl2, y2, sx2-cl1, y1)
		
		subc_line("top-silk", sx1+cl2, -y2, sx2-cl2, -y2)
		subc_line("top-silk", sx1+cl2, -y2, sx1+cl1, -y1)
		subc_line("top-silk", sx2-cl2, -y2, sx2-cl1, -y1)

		subc_rectangle("top-silk", sx1, y1, sx1+cl1, -y1, "right,NE,SE", rarc)
		subc_rectangle("top-silk", sx2-cl1, y1, sx2, -y1, "left,NW,SW", rarc)
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
			subc_line("top-silk", sx1, y, sx2, y)
		}
	}
	else if (P["type"] == "line") {
		subc_line("top-silk", -spacing/2, 0, +spacing/2, 0)
	}
	else if (P["type"] == "standing") {
		r = dia*2
		if (r < DEFAULT["pin_ringdia"]/2*1.2)
			r = DEFAULT["pin_ringdia"]/2*1.2
		subc_arc("top-silk", -spacing/2, 0, r, 0, 360)
		subc_line("top-silk", -spacing/2, 0, +spacing/2, 0)
	}
	else {
		error("Invalid type")
	}

	dimension(sx2, -dia, sx2, dia, spacing/2, "dia")

# silk wiper
	if (P["wiper"] == "thermistor") {
		x = len/3
		subc_line("top-silk", -4*x/4, dia*2, -2*x/4, dia*2)
		subc_line("top-silk", -2*x/4, dia*2, +2*x/4, -dia*2)
	}
	else if (P["wiper"] == "aarrow") {
		x = len/3
		subc_arrow("top-silk", -2*x/4, dia*2, +2*x/4, -dia*2-mil(30))
	}
	else if (P["wiper"] == "parrow") {
		subc_arrow("top-silk", 0, -dia*2-mil(30), 0, -dia)
	}
	else if (P["wiper"] == "looparrow") {
		y = -dia*2-mil(30)
		x = sx2+len/8
		subc_arrow("top-silk", 0, y, 0, -dia)
		subc_line("top-silk", 0, y, x, y)
		subc_line("top-silk", x, y, x, 0)
	}
	else if ((P["wiper"] != "none") && (P["wiper"] != "")) {
		error("Invalid wiper")
	}

# silk sign
	if (P["pol"] == "sign") {
		size=mil(10)

		oy = size*2.2-offs_y
		ox = DEFAULT["pin_ringdia"]/2+size*1.1-offs_x
		subc_line("top-silk", ox-size, oy, ox+size, oy)

		ox = spacing - (DEFAULT["pin_ringdia"]/2+size*1.1)-offs_x
		subc_line("top-silk", ox-size, oy, ox+size, oy)
		subc_line("top-silk", ox, oy-size, ox, oy+size)
	}
	else if (P["pol"] == "bar") {
		offs=DEFAULT["line_thickness"]
		subc_rectangle("top-silk", -spacing/4-offs, -dia, -spacing/4+offs, +dia,  DEFAULT["line_thickness"]*4)
	}
	else if (P["pol"] == "dot") {
		r=2*DEFAULT["line_thickness"]/3
		subc_arc("top-silk", -spacing/4-r*3, -dia/2, r, 0, 360)
	}
	else if ((P["pol"] != "") && (P["pol"] != "none")) {
		error("Invalid pol")
	}

	subc_end()
}
