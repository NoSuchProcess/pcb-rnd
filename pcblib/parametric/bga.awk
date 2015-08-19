BEGIN {
	set_arg(P, "?spacing", "0.5mm")
	set_arg(P, "?balldia", "0.35mm")
	set_arg(P, "?silkmark", "arc")

	proc_args(P, "nx,ny,spacing,balldia,silkmark,map,width,height", "")

	step = parse_dim(P["spacing"])

	half=step/2

	nx = int(P["nx"])
	ny = int(P["ny"])

	if (P["map"] != "") {
		v = split(P["map"], A, ":")
		x = 0
		y = 0
		for(n = 1; n <= v; n++) {
			if ((A[n] == "") || (A[n] == "#")) {
				x = 0
				y++
				continue;
			}
			if (x > nx)
				nx = x
			if (y > ny)
				ny = y
			print x,y,A[n] > "/dev/stderr"
			MAP[x, y] = A[n]
			x++
		}
		ny++;
	}
	else
		if ((nx == "") || (ny == ""))
			error("missing argument: need nx,ny or a map")

	balldia = parse_dim(P["balldia"])
	bw  = parse_dim(P["width"])
	bh  = parse_dim(P["height"])

	if (bw == "")
		bw = (nx+1)*step
	if (bh == "")
		bh = (ny+1)*step

	xo = (nx-1)*step/2
	yo = (ny-1)*step/2

	element_begin("", "U1", nx "*" ny   ,0,0, 0, -bh)

	for(x = 0; x < nx; x++) {
		for(y = 0; y < ny; y++) {
			xx = x * step - xo
			yy = y * step - yo
			name = MAP[x,y]
			if (name == "!")
				continue
			if (name == "")
				name = "NC"
			element_pad_circle(xx, yy, balldia, name)
		}
	}

	xx = -1 * (bw/2)
	yy = -1 * (bh/2)
	element_rectangle(xx, yy, bw/2, bh/2)

	silkmark(P["silkmark"], xx, yy, half*1.5)

	element_end()
}
