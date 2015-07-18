BEGIN {

	proc_args(P, "nx,ny,spacing,silkmark,eshift,etrunc", "nx,ny")

	set_arg(P, "?spacing", 100)
	set_arg(P, "?silkmark", "square")


	step = parse_dim(P["spacing"])

	if (pin_ringdia > step*0.9)
		pin_ringdia = step*0.9

	if (pin_drill > pin_ringdia*0.9)
		pin_drill = pin_ringdia*0.9

	half=step/2

	eshift=tolower(P["eshift"])
	etrunc=tolower(P["etrunc"])
	if ((eshift != "x") && (eshift != "y") && (eshift != ""))
		error("eshift must be x or y (got: ", eshift ")");

	element_begin(P["spacing"] " mil connector", "CONN1", P["nx"] "*" P["ny"]    ,0,0, 0, -step)

	for(x = 0; x < P["nx"]; x++) {
		if ((eshift == "x") && ((x % 2) == 1))
			yo = step/2
		else
			yo = 0
		for(y = 0; y < P["ny"]; y++) {
			if ((eshift == "y") && ((y % 2) == 1)) {
				xo = step/2
				if ((etrunc) && (x == P["nx"]-1))
					continue
			}
			else {
				xo = 0
				if ((etrunc) && (y == P["ny"]-1) && (yo != 0))
					continue
			}
			element_pin(x * step + xo, y * step + yo)
		}
	}

	xo = 0
	yo = 0
	if (!etrunc) {
		if (eshift == "y")
			xo = step/2
		if (eshift == "x")
			yo = step/2
	}

	element_rectangle(-half, -half, P["nx"] * step - half + xo, P["ny"] * step - half + yo)

	if (P["silkmark"] == "angled") {
		element_line(0, -half,  -half, 0)
	}
	else if (P["silkmark"] == "square") {
		element_line(-half, half,  half, half)
		element_line(half, -half,  half, half)
	}
	else if (P["silkmark"] == "external") {
		element_line(-half, 0,        -step, -half/2)
		element_line(-half, 0,        -step, +half/2)
		element_line(-step, -half/2,  -step, +half/2)
	}
	else if ((P["silkmark"] != "none") && (P["silkmark"] != "")) {
		error("invalid silkmark parameter: " P["silkmark"])
	}
	
	element_end()
}
