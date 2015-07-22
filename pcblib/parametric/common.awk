#@@param:pin_ringdia pin's copper ring (annulus) outer diameter (in mil by default, mm suffix can be used)
#@@param:pin_clearance pin's copper clearance diameter (in mil by default, mm suffix can be used)
#@@param:pin_mask pin's solder mask diameter (in mil by default, mm suffix can be used)
#@@param:pin_drill copper pin's drill diameter (in mil by default, mm suffix can be used)
#@@param:pad_thickness width of pads (in mil by default, mm suffix can be used)
#@@param:pad_clearance copper clearance around the pad (in mil by default, mm suffix can be used)
#@@param:pad_mask pin's solder mask (in mil by default, mm suffix can be used)
#@@param:line_thickness silk line thickness (in mil by default, mm suffix can be used)


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

# convert coord given in mm to footprint units
function mm(coord)
{
	return coord*3937
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
		if (DEFAULT[key, "dim"]) {
			if (value ~ "mm")
				value = mm(value)
			else
				value = mil(value)
		}
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

# Assume h is a dimension; if it has an "mm" suffix, convert it from mm,
# else convert it from mil.
function parse_dim(h)
{
	if (h == "")
		return ""
	if (h ~ "mm") {
		sub("mm", "", h)
		return mm(h)
	}
	return mil(h)
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
