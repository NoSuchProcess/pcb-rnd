BEGIN {
	set_arg(P, "?pad_spacing", "0.5mm")
	set_arg(P, "?ext_bloat", "0.37mm")
	set_arg(P, "?int_bloat", "0.37mm")
	set_arg(P, "?pad_thickness", "0.3mm")
	set_arg(P, "?silkmark", "dot")
	set_arg(P, "?line_thickness", "0.1mm")

	proc_args(P, "nx,ny,x_spacing,y_spacing,pad_spacing,ext_bloat,int_bloat,width,height,cpad_width,cpad_height,cpad_auto,silkmark", "nx,ny")

	nx = int(P["nx"])
	ny = int(P["ny"])
	if ((nx < 2) || (ny < 2))
		error("Number of pins have to be more than 2 in both directions")

	x_spacing=parse_dim(P["x_spacing"])
	y_spacing=parse_dim(P["y_spacing"])
	pad_spacing=parse_dim(P["pad_spacing"])
	pt = DEFAULT["pad_thickness"]
	ext_bloat=parse_dim(P["ext_bloat"]) - pt/2
	int_bloat=parse_dim(P["int_bloat"]) - pt/2
	width=parse_dim(P["width"])
	height=parse_dim(P["height"])

	if (x_spacing == "")
		x_spacing = (nx+1) * pad_spacing + 2*int_bloat
	if (y_spacing == "")
		y_spacing = (ny+1) * pad_spacing + 2*int_bloat

	cpad_width=parse_dim(P["cpad_width"])
	cpad_height=parse_dim(P["cpad_height"])

	if (tobool(P["cpad_auto"])) {
		if (cpad_width == "")
			cpad_width = x_spacing - int_bloat*2 - pt - int_bloat*2
		if (cpad_height == "")
			cpad_height = y_spacing - int_bloat*2 - pt - int_bloat*2
	}

	if (width == "")
		width = x_spacing
	if (height == "")
		height = y_spacing

	element_begin("", "U1", 2*nx + 2*ny   ,0,0, -width/2 - mm(1), -height/2 - mm(2))

	cx = (nx+1)/2
	cy = (ny+1)/2

	for(n = 1; n <= ny; n++) {
		y = (-cy + n) * pad_spacing
		x1 = -x_spacing/2
		x2 = x_spacing/2
		element_pad(x1-ext_bloat, y, x1+int_bloat, y, pad_width, n, "square")
		element_pad(x2-int_bloat, y, x2+ext_bloat, y, pad_width, nx+2*ny-n+1, "square")
		if (n == 1)
			y1 = y
		if (n == 2)
			dimension(x1, y1, x1, y, (ext_bloat * -3), "pad_spacing")
	}

	for(n = 1; n <= nx; n++) {
		x = (-cx + n) * pad_spacing
		y1 = -y_spacing/2
		y2 = y_spacing/2
		element_pad(x, y1-ext_bloat, x, y1+int_bloat, pad_width, nx*2+ny*2-n+1, "square")
		element_pad(x, y2-int_bloat, x, y2+ext_bloat, pad_width, n+ny, "square")
	}


	if ((cpad_width != "") && (cpad_height != "")) {
		element_pad_rectangle(-cpad_width/2, -cpad_height/2, +cpad_width/2, +cpad_height/2, 2*nx+2*ny+1, "square")
		dimension(-cpad_width/2, -cpad_height/2, +cpad_width/2, -cpad_height/2, "@0;" (height * -0.6-ext_bloat), "cpad_width")
		dimension(cpad_width/2, -cpad_height/2, cpad_width/2, +cpad_height/2, "@" (width * 0.8+ext_bloat) ";0", "cpad_height")
	}

	wx = (width  - nx * pad_spacing) / 3.5
	wy = (height - ny * pad_spacing) / 3.5

	element_rectangle_corners(-width/2, -height/2, width/2, height/2, wx, wy)

	dimension(-width/2, -height/2, +width/2, -height/2, "@0;" height*-0.8-ext_bloat,       "width")
	dimension(+width/2, -height/2, +width/2, +height/2, "@" (width * 1+ext_bloat) ";0",  "height")

	dimension(-width/2, -height/2, +width/2, -height/2, "@0;" height*-1-ext_bloat,       "x_spacing")
	dimension(+width/2, -height/2, +width/2, +height/2, "@" (width * 1.2+ext_bloat) ";0",  "y_spacing")

	silkmark(P["silkmark"], -width/2 - wx/2, -height/2+wy*1.5, (wx+wy)/4)


	element_end()
}
