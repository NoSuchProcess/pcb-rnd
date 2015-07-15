BEGIN {
	q="\""

	pin_ringdia = 8000
	pin_clearance = 5000
	pin_mask = 8600
	pin_drill = 3937
	pin_flags = "__auto"
	line_thickness = 1500
}

# return a if it is not empty, else return b
function either(a, b)
{
	return a != "" ? a : b
}

# generate an element header line; any argument may be empty
function element_begin(desc, name, value, cent_x, cent_y, text_x, text_y, text_dir, text_scale)
{
	print "Element[" q q, q desc q, q name q, q value q, 
	either(cent_x, 0), either(cent_y, 0),
	either(text_x, 0), either(text_y, 0),
	either(text_dir, 0), either(text_scale, 100), q q "]"
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

	print "	Pin[" x, y,
		either(ringdia, pin_ringdia), either(clearance, pin_clearance), either(mask, pin_mask),
		either(drill, pin_drill), q name q, q number q, q flags q "]"
}

# draw a line on silk; thickness is optional (default: line_thickness)
function element_line(x1, y1, x2, y2,   thickness)
{
	print "	ElementLine[" x1, y1, x2, y2, either(thickness, line_thickness) "]"
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
	return coord/25.4*10000
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
			key=A[n]
			val=A[n]
			sub("=.*", "", key)
			sub("^[^=]*=", "", val)
			OUT[key] = val
		}
		else {
#			positional
			if (N[pos] == "") {
				print "Error: too many positional arguments at " A[n] > "/dev/stderr"
				exit 1
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
			print "Error: missing argument", M[n], "(or positional " n ")"  > "/dev/stderr"
			exit 1
		}
	}
}
