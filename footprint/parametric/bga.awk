function pinalpha(p,    s)
{
	if (p >= alphabet_len)
		s = pinalpha(int(p/alphabet_len)-1)
	return s sprintf("%s", substr(alphabet, (p % alphabet_len)+1, 1))
}

function automap(algo, pivot, revx, revy   ,xx,yy)
{
	if (algo == 1) {
		for(y = 0; y < ny; y++) {
			if (revy)
				yy = ny - y - 1
			else
				yy = y
			for(x = 0; x < nx; x++) {
				if (revx)
					xx = nx - x - 1
				else
					xx = x
				if (pivot)
					MAP[x,y] = pinalpha(xx) yy+1
				else
					MAP[x,y] = pinalpha(yy) xx+1
			}
		}
	}
}

BEGIN {
	help_auto()
	set_arg(P, "?spacing", "0.5mm")
	set_arg(P, "?balldia", "0.35mm")
	set_arg(P, "?silkmark", "arc")

	proc_args(P, "nx,ny,spacing,balldia,silkmark,map,width,height,automap,automap2,alphabet", "")

	step = parse_dim(P["spacing"])

	half=step/2

	nx = int(P["nx"])
	ny = int(P["ny"])

	alphabet = P["alphabet"]
	if (alphabet == "")
		alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	alphabet_len = length(alphabet)

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
		nx++;
	}
	else {
		if ((nx == "") || (ny == ""))
			error("missing argument: need nx,ny or a map")
		if (P["automap"] ~ "alnum")
			automap(1, (P["automap2"] ~ "pivot"), (P["automap2"] ~ "reversex"), (P["automap2"] ~ "reversey"))
		else if ((P["automap"] ~ "none") || (P["automap"] == "")) {
		}
		else
			error("automap should be alnum or none")
	}

	balldia = parse_dim(P["balldia"])
	bw  = parse_dim(P["width"])
	bh  = parse_dim(P["height"])

	if (bw == "")
		bw = (nx+1)*step
	if (bh == "")
		bh = (ny+1)*step

	xo = (nx-1)*step/2
	yo = (ny-1)*step/2

	subc_begin(nx "*" ny, "U1", 0, -bh)

	proto = subc_proto_create_pad_circle(balldia)

	for(x = 0; x < nx; x++) {
		for(y = 0; y < ny; y++) {
			xx = x * step - xo
			yy = y * step - yo
			name = MAP[x,y]
			if (name == "!")
				continue
			if (name == "")
				name = "NC"
			subc_pstk(proto, xx, yy, 0, name)
		}
	}

	dimension(-xo, -yo, -xo+step, -yo, bw/2, "spacing")
	dimension(-xo-balldia/2, +yo, -xo+balldia/2, +yo, -bw*0.75, "balldia")


	xx = -1 * (bw/2)
	yy = -1 * (bh/2)
	subc_rectangle("top-silk", xx, yy, bw/2, bh/2)

	dimension(xx, yy, bw/2, yy, bw/2, "width")
	dimension(bw/2, yy, bw/2, bh/2, +bh/2, "height")

	silkmark(P["silkmark"], xx, yy, half*1.5)

	subc_end()
}
