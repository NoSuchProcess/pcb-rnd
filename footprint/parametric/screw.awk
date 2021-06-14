function shp(r, edges, tx      ,a,x,y,xl,yl,step,x1,y1,x2,y2,tx1,tx2,txs)
{
	step = 2*3.141592654/edges
	if (tx == 1) {
		tx1 = 0.7
		tx2 = 0.6
		txs = 5
	}
	else if (tx == 2) {
		tx1 = 0.85
		tx2 = 0.8
		txs = 5
	}
	else if (tx == 3) {
		tx1 = 0.2
		tx2 = 0.2
		txs = 3
	}
	for(n = 0; n <= edges; n++) {
		a += step
		x = cos(a)*r
		y = sin(a)*r
		if (xl != "") {
			if (tx) {
				x1 = cos(a-(txs-1)*step/txs)*r*tx1
				y1 = sin(a-(txs-1)*step/txs)*r*tx1
				x2 = cos(a-step/txs)*r*tx1
				y2 = sin(a-step/txs)*r*tx1
				x3 = cos(a-step/2)*r*tx2
				y3 = sin(a-step/2)*r*tx2
				subc_line("top-silk", xl, yl, x1, y1)
				subc_line("top-silk", x, y,   x2, y2)
				subc_line("top-silk", x3, y3, x1, y1)
				subc_line("top-silk", x3, y3, x2, y2)
			}
			else
				subc_line("top-silk", xl, yl, x, y)
		}
		xl = x
		yl = y
	}
}

function round_up(num, to)
{
	if ((num/to) == int(num/to))
		return num
	return int(num/to+1)*to
}

BEGIN {
	help_auto()
	set_arg(P, "?shape", "circle")
	proc_args(P, "hole,head,shape,ring", "hole")

	subc_begin("screw:" P["hole"] "," P["head"]"," P["shape"], "S1", 0, -mil(100), 0)

	if (P["hole"] ~ "^M") {
		hole = P["hole"]
		sub("^M", "", hole)
		h = parse_dim(int(hole) "mm")
		if ((hole ~ "tight") || (hole ~ "close.fit"))
			hole = h * 1.05
		else
			hole = h * 1.1
		hd = parse_dim(P["head"])
		if ((hd == 0) || (hd == "")) {
			hd = P["head"]
			if (hd == "button")
				head = 1.9*h 
			else if (hd == "button")
				head = 1.9*h 
			else if (hd == "cheese")
				head = round_up(1.7*h, mm(0.5))
			else if (hd ~ "flat.washer")
				head = round_up(2.1*h, mm(1))
			else if ((hd == "") || (hd == "pan") || (hd ~ "int.*.lock.washer"))
				head = 2*h
			else
				error("Unknown standard head: " hd)
		}
		else
			head = hd
#		print hole, head > "/dev/stderr"
	}
	else {
		hole = parse_dim(P["hole"])
		head = parse_dim(P["head"])
	}

	if (head == "")
		error("need a standard screw name, e.g. M3, or a head diameter")

	if (head < hole)
		error("head diameter must be larger than hole diameter")

	ring = parse_dim(P["ring"])

	if (ring == "")
		ring = head*0.8


	proto = subc_proto_create_pin_round(hole, ring)
	subc_pstk(proto, 0, 0, 0, 1)


	shape = ":" P["shape"] ":"

	if (shape ~ ":circle:")
		subc_arc("top-silk", 0, 0, head/2, 0, 360)

	if (shape ~ ":hex:")
		shp(head/2, 6, 0)

	if (shape ~ ":tx:")
		shp(head/2, 6, 1)

	if (shape ~ ":xzn:")
		shp(head/2, 12, 2)

	if (shape ~ ":ph:")
		shp(head*0.4, 4, 3)

	if (shape ~ ":slot:")
		subc_line("top-silk", -head/2, 0, head/2, 0)

	dimension(-head/2, 0, head/2, 0, head*0.7, "head")
	dimension(-hole/2, 0, hole/2, 0, head*0.6, "hole")

	subc_end()
}
