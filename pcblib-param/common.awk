BEGIN {
	q="\""

	pin_ringdia = 8000
	pin_clearance = 5000
	pin_mask = 8600
	pin_drill = 3937
	pin_flags = "__auto"
	line_thickness = 1500
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
function element_pin(x, y,   ringdia, clearance, mask, drill, name, number, flags)
{
	if (number == "")
		number = ++pin_number

	flags = either(flags, pin_flags)

	if (flags == "__auto") {
		if (number == 1)
			flags = "square"
		else
			flags = ""
	}

	print "	Pin[" int(x), int(y),
		int(either(ringdia, pin_ringdia)), int(either(clearance, pin_clearance)), int(either(mask, pin_mask)),
		int(either(drill, pin_drill)), q name q, q number q, q flags q "]"
}

# draw a line on silk; thickness is optional (default: line_thickness)
function element_line(x1, y1, x2, y2,   thickness)
{
	print "	ElementLine[" int(x1), int(y1), int(x2), int(y2), int(either(thickness, line_thickness)) "]"
}

# draw a rectangle of silk lines 
function element_rectangle(x1, y1, x2, y2,    thickness)
{
	element_line(x1, y1, x1, y2, thickness)
	element_line(x1, y1, x2, y1, thickness)
	element_line(x2, y2, x1, y2, thickness)
	element_line(x2, y2, x2, y1, thickness)
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


# Process a generator argument list from arg_names. Save the result in
# array OUT. If mandatory is specified, check whether all mandatory
# parameters are specified
# Both arg_names and mandatory are comma separated list of argument names
function proc_args(OUT, arg_names,   mandatory,  N,A,M,v,n,key,val,pos)
{
	sub(" ", "", arg_names)
	sub(" ", "", mandatory)
	split(arg_names, N, ",")
	v = split(args, A, ",")

# fill in all named and positional arguments
	pos = 1
	for(n = 1; n <= v; n++) {
		if (A[n] ~ "=") {
#			named
			key=strip(A[n])
			val=strip(A[n])
			sub("=.*", "", key)
			sub("^[^=]*=", "", val)
			OUT[key] = val
		}
		else {
#			positional
			A[n] = strip(A[n])
			if (N[pos] == "") {
				error("too many positional arguments at " A[n])
			}
			while(N[pos] in OUT) pos++
			OUT[N[pos]] = A[n]
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
