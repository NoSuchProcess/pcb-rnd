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

# figure if we need to export openscad map (has any pin missing)
function scadmap_need(     x,y)
{
	for(x = 0; x < nx; x++)
		for(y = 0; y < ny; y++)
			if (MAP[x,y] == "!")
				return 1;
	return 0;
}

function scadmap(     x,y,s)
{

	if (!scadmap_need())
		return "";


	s = ",omit_map=["
	for(x = 0; x < nx; x++) {
		if (x > 0) s = s ","
		s = s "[";
		for(y = 0; y < ny; y++) {
			if (y > 0) s = s ","
			s = s (MAP[x,y] == "!")
		}
		s = s "]"
	}
	s = s "]"
	return s
}

BEGIN {
	help_auto()
	set_arg(P, "?spacing", "0.5mm")
	set_arg(P, "?balldia", "0.35mm")
	set_arg(P, "?silkmark", "arc")

	proc_args(P, "nx,ny,spacing,balldia,silkmark,map,width,height,automap,automap2,alphabet,ballmask,ballpaste", "")

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
			if (x >= nnx)
				nnx = x+1
			if (y >= nny)
				nny = y+1
#			print x,y,A[n] > "/dev/stderr"
			MAP[x, y] = A[n]
			x++
		}
#print "n:" nx, ny, "nn:", nnx, nny > "/dev/stderr"
		if (nnx > nx)
			nx = nnx;
		if (nny > ny)
			ny = nny;
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

# safe defaults on ball mask and paste
	if (P["ballmask"] == "")
		ballmask = balldia * 1.2
	else
		ballmask = parse_dim(P["ballmask"])
	if (P["ballpaste"] == "")
		ballpaste = balldia * 0.8
	else
		ballpaste = parse_dim(P["ballpaste"])

	bw  = parse_dim(P["width"])
	bh  = parse_dim(P["height"])

	if (bw == "")
		bw = (nx+1)*step
	if (bh == "")
		bh = (ny+1)*step

	xo = (nx-1)*step/2
	yo = (ny-1)*step/2

	slant = parse_dim(P["3dslant"])
	if (slant != 0)
		slant = ",slant=" rev_mm(slant)

	SCAT["openscad"]="bga.scad";
	SCAT["openscad-param"]="nx=" nx ", ny=" ny ",balldia=" rev_mm(balldia) ",width=" rev_mm(bw) ",height=" rev_mm(bh) slant scadmap();

	subc_begin(nx "*" ny, "U1", 0, -bh, "", SCAT)

	proto = subc_proto_create_pad_circle(balldia, ballmask, ballpaste)

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
