BEGIN {
	base_unit_mm = 0

	help_auto()
	set_arg(P, "?spacing", 100)
	set_arg(P, "?silkmark", "square")
	set_arg(P, "?sequence", "normal")

	proc_args(P, "nx,ny,spacing,silkmark,eshift,etrunc", "nx,ny")


	step = parse_dim(P["spacing"])

	if (pin_ringdia > step*0.9)
		pin_ringdia = step*0.9

	if (pin_drill > pin_ringdia*0.9)
		pin_drill = pin_ringdia*0.9

	half=step/2

	P["nx"] = int(P["nx"])
	P["ny"] = int(P["ny"])

	eshift=tolower(P["eshift"])
	etrunc=tobool(P["etrunc"])
	if ((eshift != "x") && (eshift != "y") && (eshift != "") && (eshift != "none"))
		error("eshift must be x or y or none (got: " eshift ")");

	subc_begin(P["nx"] "*" P["ny"], "CONN1", 0, -step)

	proto_s = subc_proto_create_pin_square()
	proto_r = subc_proto_create_pin_round()

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
			if (P["sequence"] == "normal") {
				pinno++
			}
			else if (P["sequence"] == "pivot") {
				pinno = y * P["nx"] + x + 1
			}
			else if (P["sequence"] == "zigzag") {
				if (x % 2) {
					pinno = (x+1) * P["ny"] - y
					if ((etrunc) && (eshift == "x"))
						pinno -= int(x/2)+1
					else if ((etrunc) && (eshift == "y"))
						pinno += int(x/2)
				}
				else {
					pinno = x * P["ny"] + y + 1
					if ((etrunc) && (eshift == "x"))
						pinno -= x/2
					else if ((etrunc) && (eshift == "y"))
						pinno += x/2-1
				}
			}
			subc_pstk(pinno == 1 ? proto_s : proto_r, x * step + xo, y * step + yo, 0, pinno)
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

	subc_rectangle("top-silk", -half, -half, P["nx"] * step - half + xo, P["ny"] * step - half + yo)

	silkmark(P["silkmark"], -half, -half, half)

	dimension(0, step, step, step, step*2, "spacing")

	subc_end()
}
