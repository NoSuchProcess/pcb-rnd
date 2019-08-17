function pol_sign(   ox)
{
	size=mil(20)

	ox = dia/2+size*2
	subc_line("top-silk", ox-size, 0, ox+size, 0)

	ox = -dia/2-size*2
	subc_line("top-silk", ox-size, 0, ox+size, 0)
	subc_line("top-silk", ox, -size, ox, size)
}

BEGIN {
	base_unit_mm = 0

	help_auto()
	set_arg(P, "?pol", "sign")
	proc_args(P, "spacing,pol,dia", "spacing")

	spacing = parse_dim(P["spacing"])
	dia = either(parse_dim(P["dia"]), spacing*2)

	offs_x = +spacing/2

	subc_begin("rcy" P["spacing"], "C1", -spacing/5, -mil(20), 0)

	proto_s = subc_proto_create_pin_square()
	proto_r = subc_proto_create_pin_round()

	subc_pstk(proto_s, -spacing/2, 0, 0, 1)
	subc_pstk(proto_r, +spacing/2, 0, 0, 2)

	dimension(-spacing/2, 0, +spacing/2, 0, dia*0.8, "spacing")


# silk rectangle and pins
	subc_arc("top-silk", 0, 0, dia/2, 0, 360)
	dimension(-dia/2, 0, +dia/2, 0, dia*-0.8, "dia")


	if (P["pol"] == "sign") {
		pol_sign()
	}
	else if (P["pol"] ~ "^bar") {
# determine bar side (default to -)
		side=P["pol"]
		sub("^bar", "", side)
		if (side ~ "sign") {
			pol_sign()
			sub("sign", "", side)
		}
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
				subc_line("top-silk", side*xs, y, side*x, y, th)
				if (y != 0)
					subc_line("top-silk", side*xs, -y, side*x, -y, th)
			}
			else {
# keep out a rectangle around the pin
				end1=spacing/2-ring
				end2=spacing/2+ring
				if (end1 > xs)
					subc_line("top-silk", side*xs, y, side*end1, y, th)
				if (end2 < x)
					subc_line("top-silk", side*end2, y, side*x, y, th)
				if (y != 0) {
					if (end1 > xs)
						subc_line("top-silk", side*xs, -y, side*end1, -y, th)
					if (end2 < x)
						subc_line("top-silk", side*end2, -y, side*x, -y, th)
				}
			}
		}
	}
	else if ((P["pol"] != "") && (P["pol"] != "none")) {
		error("Invalid pol")
	}

	subc_end()
}
