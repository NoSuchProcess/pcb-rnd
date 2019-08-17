### DO NOT USE THIS FILE ###

# This file is provided only for compatibility with old parametric footprints.
# New parametric footprints should use common_subc.awk because that uses lihata
# subcircuits instead of obsolete element format.

#@@param:pin_ringdia pin's copper ring (annulus) outer diameter (in mil by default, mm suffix can be used)
#@@optional:pin_ringdia
#@@dim:pin_ringdia

#@@param:pin_clearance pin's copper clearance diameter (in mil by default, mm suffix can be used)
#@@optional:pin_clearance
#@@dim:pin_clearance

#@@param:pin_mask pin's solder mask diameter (in mil by default, mm suffix can be used)
#@@optional:pin_mask
#@@dim:pin_mask

#@@param:pin_drill copper pin's drill diameter (in mil by default, mm suffix can be used)
#@@optional:pin_drill
#@@dim:pin_drill

#@@param:pad_thickness width of pads (in mil by default, mm suffix can be used)
#@@optional:pad_thickness
#@@dim:pad_thickness

#@@param:pad_clearance copper clearance around the pad (in mil by default, mm suffix can be used)
#@@optional:pad_clearance
#@@dim:pad_clearance

#@@param:pad_mask pin's solder mask (in mil by default, mm suffix can be used)
#@@optional:pad_mask
#@@dim:pad_mask

#@@param:line_thickness silk line thickness (in mil by default, mm suffix can be used)
#@@optional:line_thickness
#@@dim:line_thickness

BEGIN {
	q="\""

	DEFAULT["pin_ringdia"] = 8000
	DEFAULT["pin_ringdia", "dim"] = 1

	DEFAULT["pin_clearance"] = 5000
	DEFAULT["pin_clearance", "dim"] = 1

	DEFAULT["pin_mask"] = 8600
	DEFAULT["pin_mask", "dim"] = 1

	DEFAULT["pin_drill"] = 3937
	DEFAULT["pin_drill", "dim"] = 1

	DEFAULT["line_thickness"] = 1000
	DEFAULT["line_thickness", "dim"] = 1

	DEFAULT["pad_thickness"] = 2000
	DEFAULT["pad_thickness", "dim"] = 1
	DEFAULT["pad_clearance"] = 1000
	DEFAULT["pad_clearance", "dim"] = 1
	DEFAULT["pad_mask"] = 3000
	DEFAULT["pad_mask", "dim"] = 1

	DEFAULT["pin_flags"] = "__auto"

	s_default=1
	s_weak=2
	s_explicit=3
	
	offs_x = 0
	offs_y = 0

	pi=3.141592654
}

# Throw an error and exit
function error(msg)
{
	print "Error: " msg > "/dev/stderr"
	exit 1
}

# return a if it is not empty, else return b
function either(a, b)
{
	return a != "" ? a : b
}

# strip leading/trailing whitespaces
function strip(s)
{
	sub("^[ \t\r\n]*", "", s)
	sub("[ \t\r\n]*$", "", s)
	return s
}

# translate coordinates
function coord_x(x) { return int(x + offs_x) }
function coord_y(y) { return int(y + offs_y) }

# generate an element header line; any argument may be empty
function element_begin(desc, name, value, cent_x, cent_y, text_x, text_y, text_dir, text_scale)
{
	if (desc == "") {
		desc = gen "(" args ")"
		gsub("[\r\n\t ]*[?][^,]*,[\r\n\t ]*", "", desc)
		gsub("[\r\n\t]", " ", desc)
	}
	print "Element[" q q, q desc q, q name q, q value q, 
	int(either(cent_x, 0)), int(either(cent_y, 0)),
	int(either(text_x, 0)), int(either(text_y, 0)),
	int(either(text_dir, 0)), int(either(text_scale, 100)), q q "]"
	print "("
}

# generate an element footer line
function element_end()
{
	print ")"
}

# generate a pin; arguments from ringdia are optional (defaults are in global vars pin_*)
function element_pin(x, y,  number, flags,   ringdia, clearance, mask, drill, name)
{
	if (number == "")
		number = ++pin_number

	flags = either(flags, DEFAULT["pin_flags"])

	if (flags == "__auto") {
		if (number == 1)
			flags = "square"
		else
			flags = ""
	}

	if (flags == "none")
		flags = ""

	print "	Pin[" coord_x(x), coord_y(y),
		int(either(ringdia, DEFAULT["pin_ringdia"])), int(either(clearance, DEFAULT["pin_clearance"])), int(either(mask, DEFAULT["pin_mask"])),
		int(either(drill, DEFAULT["pin_drill"])), q name q, q number q, q flags q "]"
}

# draw element pad
function element_pad(x1, y1, x2, y2, thickness,   number, flags,   clearance, mask, name)
{
	print "	Pad[", coord_x(x1), coord_y(y1), coord_x(x2), coord_y(y2), int(either(thickness, DEFAULT["pad_thickness"])),
		int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
		q name q, q number q, q flags q "]"
}

# draw element pad - no thickness, but exact corner coordinates given
function element_pad_rectangle(x1, y1, x2, y2,   number, flags,   clearance, mask, name,     th,dx,dy,cx,cy)
{
	if (x2 < x1) {
		th = x2
		x2 = x1
		x1 = th
	}
	if (y2 < y1) {
		th = y2
		y2 = y1
		y1 = th
	}

	dx = x2-x1
	dy = y2-y1

	if (dx >= dy) {
		th = dy
		cy = (y1+y2)/2

		print "	Pad[", coord_x(x1)+th/2, coord_y(cy), coord_x(x2)-th/2, coord_y(cy), th,
			int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
			q name q, q number q, q flags q "]"
	}
	else {
		th = dx
		cx = (x1+x2)/2

		print "	Pad[", coord_x(cx), coord_y(y1)+th/2, coord_x(cx), coord_y(y2)-th/2, th,
			int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
			q name q, q number q, q flags q "]"
	}
}

# draw a matrix of pads; top-left corner is x1;y1, there are nx*ny pads
# of w*h size. rows/cols of pads are drawn with ox and oy offset
function element_pad_matrix(x1, y1, nx, ny, w, h, ox, oy,     number, flags,   clearance, mask, name,     ix,iy)
{
	for(iy = 0; iy < ny; iy++)
		for(ix = 0; ix < nx; ix++)
			element_pad_rectangle(x1+ix*ox, y1+iy*oy, x1+ix*ox+w, y1+iy*oy+h, number, flags, clearance, mask, name)
}

# draw element pad circle
function element_pad_circle(x1, y1, radius,   number,  clearance, mask, name)
{
	print "	Pad[", coord_x(x1), coord_y(y1), coord_x(x1), coord_y(y1), int(either(radius, DEFAULT["pad_thickness"])),
		int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
		q name q, q number q, q "" q "]"
}

# draw a line on silk; thickness is optional (default: line_thickness)
function element_line(x1, y1, x2, y2,   thickness)
{
	print "	ElementLine[" coord_x(x1), coord_y(y1), coord_x(x2), coord_y(y2), int(either(thickness, DEFAULT["line_thickness"])) "]"
}

function element_arrow(x1, y1, x2, y2,  asize,  thickness   ,vx,vy,nx,ny,len,xb,yb)
{
	element_line(x1, y1, x2,y2, thickness)

	if (asize == 0)
		asize = mil(20)

	vx = x2-x1
	vy = y2-y1
	len = sqrt(vx*vx+vy*vy)
	if (len != 0) {
		vx /= len
		vy /= len
		nx = vy
		ny = -vx
		xb = x2 - vx*asize
		yb = y2 - vy*asize
#		element_line(x2, y2, xb + 1000, yb + 1000)
		element_line(x2, y2, xb + nx*asize/2, yb + ny*asize/2)
		element_line(x2, y2, xb - nx*asize/2, yb - ny*asize/2)
		element_line(xb - nx*asize/2, yb - ny*asize/2, xb + nx*asize/2, yb + ny*asize/2)
	}
}

# draw a rectangle of silk lines 
# omit sides as requested in omit
# if r is non-zero, round corners - omit applies as NW, NW, SW, SE
# if omit includes "arc", corners are "rounded" with lines
function element_rectangle(x1, y1, x2, y2,    omit,  r, thickness   ,tmp,r1,r2)
{
# order coords for round corners
	if (x1 > x2) {
		tmp = x1
		x1 = x2
		x2 = tmp
	}
	if (y1 > y2) {
		tmp = y1
		y1 = y2
		y2 = tmp
	}

	if (!(omit ~ "left")) {
		r1 = (omit ~ "NW") ? 0 : r
		r2 = (omit ~ "SW") ? 0 : r
		element_line(x1, y1+r1, x1, y2-r2, thickness)
	}
	if (!(omit ~ "top")) {
		r1 = (omit ~ "NW") ? 0 : r
		r2 = (omit ~ "NE") ? 0 : r
		element_line(x1+r1, y1, x2-r2, y1, thickness)
	}
	if (!(omit ~ "bottom")) {
		r1 = (omit ~ "SE") ? 0 : r
		r2 = (omit ~ "SW") ? 0 : r
		element_line(x2-r1, y2, x1+r2, y2, thickness)
	}
	if (!(omit ~ "right")) {
		r1 = (omit ~ "SE") ? 0 : r
		r2 = (omit ~ "NE") ? 0 : r
		element_line(x2, y2-r1, x2, y1+r2, thickness)
	}

	if (r > 0) {
		if (omit ~ "arc") {
			if (!(omit ~ "NW"))
				element_line(x1, y1+r, x1+r, y1)
			if (!(omit ~ "SW"))
				element_line(x1, y2-r, x1+r, y2)
			if (!(omit ~ "NE"))
				element_line(x2, y1+r, x2-r, y1)
			if (!(omit ~ "SE"))
				element_line(x2, y2-r, x2-r, y2)
		}
		else {
			if (!(omit ~ "NW"))
				element_arc(x1+r, y1+r, r, r, 270, 90)
			if (!(omit ~ "SW"))
				element_arc(x1+r, y2-r, r, r, 0, 90)
			if (!(omit ~ "NE"))
				element_arc(x2-r, y1+r, r, r, 180, 90)
			if (!(omit ~ "SE"))
				element_arc(x2-r, y2-r, r, r, 90, 90)
		}
	}
}

# draw a rectangle corners of silk lines, wx and wy long in x and y directions
# omit sides as requested in omit: NW, NW, SW, SE
# corners are always sharp
function element_rectangle_corners(x1, y1, x2, y2, wx, wy,    omit,  thickness   ,tmp)
{
	if (!(omit ~ "NW")) {
		element_line(x1, y1,     x1+wx, y1,   thickness)
		element_line(x1, y1,     x1, y1+wy,   thickness)
	}
	if (!(omit ~ "NE")) {
		element_line(x2-wx, y1,  x2, y1,    thickness)
		element_line(x2, y1,     x2, y1+wy, thickness)
	}
	if (!(omit ~ "SW")) {
		element_line(x1, y2,     x1+wx, y2, thickness)
		element_line(x1, y2-wy,  x1, y2,    thickness)
	}
	if (!(omit ~ "SE")) {
		element_line(x2-wx, y2,  x2, y2,   thickness)
		element_line(x2, y2-wy,  x2, y2,   thickness)
	}
}

# draw a line on silk; thickness is optional (default: line_thickness)
function element_arc(cx, cy, rx, ry, a_start, a_delta,   thickness)
{
	print "	ElementArc[" coord_x(cx), coord_y(cy), int(rx), int(ry), int(a_start), int(a_delta), int(either(thickness, DEFAULT["line_thickness"])) "]"
}


# convert coord given in mils to footprint units
function mil(coord)
{
	return coord * 100
}

# reverse mil(): converts footprint units back to mil
function rev_mil(coord)
{
	return coord/100
}


# convert coord given in mm to footprint units
function mm(coord)
{
	return coord*3937
}

# reverse mm(): converts footprint units back to mm
function rev_mm(coord)
{
	return coord/3937
}


function set_arg_(OUT, key, value, strength)
{
	if (OUT[key, "strength"] > strength)
		return

	OUT[key] = value
	OUT[key, "strength"] = strength
}

# set parameter key=value with optioal strength (s_* consts) in array OUT[]
# set only if current strength is stronger than the original value
# if key starts with a "?", use s_weak
# if key is in DEFAULT[], use DEFAULT[] instead of OUT[]
function set_arg(OUT, key, value     ,strength)
{
	if (strength == "")
		strength = s_explicit
	if (key ~ "^[?]") {
		sub("^[?]", "", key)
		strength = s_weak
	}

	if (key in DEFAULT) {
		if (DEFAULT[key, "dim"])
			value = parse_dim_(value, 0)
		set_arg_(DEFAULT, key, value, strength)
	}
	else
		set_arg_(OUT, key, value, strength)
}

# Process a generator argument list from arg_names. Save the result in
# array OUT. If mandatory is specified, check whether all mandatory
# parameters are specified
# Both arg_names and mandatory are comma separated list of argument names
function proc_args(OUT, arg_names,   mandatory,  N,A,M,v,n,key,val,pos)
{
	gsub(" ", "", arg_names)
	gsub(" ", "", mandatory)
	split(arg_names, N, ",")
	v = split(args, A, ",")

# fill in all named and positional arguments
	pos = 1
	for(n = 1; n <= v; n++) {
		A[n] = strip(A[n])
		if (A[n] == "")
			continue
		if (A[n] ~ "=") {
#			named
			key=A[n]
			val=A[n]
			sub("=.*", "", key)
			sub("^[^=]*=", "", val)
			set_arg(OUT, key, val, s_explicit)
		}
		else {
#			positional
			if (N[pos] == "") {
				error("too many positional arguments at " A[n])
			}
			while((N[pos] in OUT) && (N[pos, "strength"] == s_explicit)) pos++
			set_arg(OUT, N[pos], A[n], s_explicit)
			pos++
		}
	}

# check whether all mandatory arguments are specified
	v = split(mandatory, M, ",")
	for(n = 1; n <= v; n++) {
		if (!(M[n] in OUT)) {
			error("missing argument " M[n] " (or positional " n ")")
			exit 1
		}
	}
}

function parse_dim_(h, fallback_mil)
{
	if (h == "")
		return ""
	if (h ~ "mm$") {
		sub("mm", "", h)
		return mm(h)
	}
	if (h ~ "um$") {
		sub("um", "", h)
		return mm(h)/1000
	}
	if (h ~ "nm$") {
		sub("nm", "", h)
		return mm(h)/1000000
	}
	if (h ~ "cm$") {
		sub("cm", "", h)
		return mm(h)*10
	}
	if (h ~ "m$") {
		sub("m", "", h)
		return mm(h)*1000
	}
	if (h ~ "km$") {
		sub("km", "", h)
		return mm(h)*1000000
	}

	if (h ~ "in$") {
		sub("in", "", h)
		return mil(h)*1000
	}
	if (h ~ "dmil$") {
		sub("dmil", "", h)
		return mil(h)/10
	}
	if (h ~ "cmil$") {
		sub("cmil", "", h)
		return mil(h)/100
	}
	if (h ~ "mil$") {
		sub("mil", "", h)
		return mil(h)
	}
	if (fallback_mil)
		return mil(h)
	else
		return h
}

# Assume h is a dimension and convert it
function parse_dim(h)
{
	return parse_dim_(h, 1)
}

# Draw a DIP outline: useful for any rectangular package with a little
# half circle centered on the top line
#  arcr: radius of the half circle
#  xhalf: optional coordinate where the circle should be put
function dip_outline(x1, y1, x2, y2, arcr   ,xhalf)
{
	if (xhalf == "")
		xhalf=(x1+x2)/2

	element_rectangle(x1, y1, x2, y2, "top")
	element_line(x1, y1, xhalf-arcr, y1)
	element_line(xhalf+arcr, y1, x2, y1)

	element_arc(xhalf, y1,  arcr, arcr,  0, 180)
}

# decide whether x is true or false
# returns 1 if true
# returns 0 if false
function tobool(x)
{
	if (x == int(x))
		return (int(x) != 0)

	x = tolower(x)
	return (x == "true") || (x == "yes") || (x == "on")
}

# default pin1 mark on a box
# style: mark style, ":" separated list
# x,y: the coordinates of the origin corner (top-left)
# half: half the stepping of the pin grid - the size of the mark
# step: optional size of the external arrow or square (defaults to 2*half)
function silkmark(style, x, y, half,    step,   S,n,v)
{
	if (step == "")
	step = half*2

	v = split(style, S, ":")

	for(n = 1; n <= v; n++) {
		if (S[n] == "angled") {
			element_line(x+half, y,  x, y+half)
		}
		else if (S[n] == "square") {
			element_line(x, y+step,  x+2*half, y+step)
			element_line(x+step, y,  x+2*half, y+step)
		}
		else if ((S[n] == "external") || (S[n] == "externalx")) {
			element_line(x, y+half,         x-step+half, y+half/2)
			element_line(x, y+half,         x-step+half, y+half*1.5)
			element_line(x-step+half, y+half/2,         x-step+half, y+half*1.5)
		}
		else if (S[n] == "externaly") {
			element_line(x+half, y,         x-half/2+half, y-step+half)
			element_line(x+half, y,         x+half/2+half, y-step+half)
			element_line(x-half/2+half, y-step+half,  x+half/2+half, y-step+half)
		}
		else if (S[n] == "external45") {
			element_line(x, y,       x-half, y-half/3)
			element_line(x, y,       x-half/3, y-half)
			element_line(x-half, y-half/3, x-half/3, y-half)
		}
		else if (S[n] == "arc") {
			element_arc(x, y, step/2, step/2, 180, 270)
		}
		else if (S[n] == "circle") {
			element_arc(x, y, step/2, step/2, 0, 360)
		}
		else if (S[n] == "dot") {
			element_arc(x-step/2, y-step/2, step/4, step/4, 0, 360)
		}
		else if ((S[n] != "none") && (S[n] != "")) {
			error("invalid silkmark parameter: " S[n])
		}
	}
}

# output a dimension specification between x1;y1 and x2;y2, text distance at dist
# for a name,value pair
# if name is empty, only value is printed
# if value is empty, it's calculated
# if only name should be printed, value should be "!"
# if dist starts with a "@", it's the absolute coordinate of the center of the dim line (text base), else it's relative distance from the measured line
function dimension(x1, y1, x2, y2, dist, name,    value,    vx,vy)
{
	print "#dimension", coord_x(x1), coord_y(y1), coord_x(x2), coord_y(y2), dist, name, value
}

function help_extract(SEEN, fn, dirn, OVER, IGN,     WANT,tmp,key,val,i,skip)
{
	if (fn in SEEN)
		return
	SEEN[fn]++
	print "#@@info-gen-extract " fn
	close(fn)
	while((getline line < fn) > 0) {
		if (line ~ "^#@@include") {
			sub("^#@@include[ \t]*", "", line)
			tmp = dirn "/" line
			WANT[tmp]++
		}
		else if (line ~ "^#@@over@ignore") {
			key = line
			sub("^#@@over@ignore:", "", key)
			sub(" .*", "", key)
			IGN[key] = 1
		}
		else if (line ~ "^#@@over@") {
			key = line
			sub("^#@@over@", "", key)
			val = "#@@" key
			sub(" .*", "", key)
			OVER[key] = val
		}
		else if (line ~ "^#@@") {
			key = line
			sub("^#@@", "", key)
			sub(" .*", "", key)
			skip = 0
			for(i in IGN) {
				if (key ~ i)
					skip = 1
			}
			if (skip)
				continue
			if (key in OVER) {
				print OVER[key]
				OVER[key "::PRINTED"] = 1
			}
			else
				print line
		}
	}
	close(fn)
	for(tmp in WANT)
		help_extract(SEEN, tmp, dirn, OVER, IGN)
}

function help_print(   SEEN, OVER, dirn, k)
{
	print "#@@info-generator pcblib common.awk"
	dirn = genfull
	sub("/[^/]*$", "", dirn)
	help_extract(SEEN, genfull, dirn, OVER)
	for(k in OVER) {
		if (!(k ~ "::PRINTED$") && !((k "::PRINTED") in OVER))
			print OVER[k]
	}
}

function help_auto()
{
	if ((args ~ "^--help") || (args ~ ",[ \t]*--help")) {
		help_print()
		exit(0)
	}
}
