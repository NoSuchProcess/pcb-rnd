BEGIN {
	q="\""

	pin_ringdia = 8000
	pin_clearance = 5000
	pin_mask = 8600
	pin_drill = 3937
	pin_flags = "__auto"
	line_thickness = 1500
}

function either(a, b)
{
	return a != "" ? a : b
}

Element["" "" "" "" 85000 145000 0 0 0 100 ""]

function element_begin(desc, name, value, cent_x, cent_y, text_x, text_y, text_dir, text_scale)
{
	print "Element[" q q, q desc q, q name q, q value q, 
	either(cent_x, 0), either(cent_y, 0),
	either(text_x, 0), either(text_y, 0),
	either(text_dir, 0), either(text_scale, 100), q q "]"
	print "("
}

function element_end()
{
	print ")"
}

function element_pin(x, y, ringdia, clearance, mask, drill, name, number, flags)
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

function element_line(x1, y1, x2, y2, thickness)
{
	print "	ElementLine[" x1, y1, x2, y2, either(thickness, line_thickness) "]"
}

function element_rectangle(x1, y1, x2, y2, thickness)
{
	element_line(x1, y1, x1, y2, thickness)
	element_line(x1, y1, x2, y1, thickness)
	element_line(x2, y2, x1, y2, thickness)
	element_line(x2, y2, x2, y1, thickness)
}

function mil(coord)
{
	return coord * 100
}
