function qf_globals(pre_args, post_args    ,reqs)
{
	DEFAULT["cpad_mask", "dim"] = 1

	if (hook_spc_conv == "")
		hook_spc_conv = 1.8
	if (hook_cpad_mult == "")
		hook_cpad_mult = 1

	if (!qf_no_defaults) {
		set_arg(P, "?pad_spacing", "0.5mm")
		set_arg(P, "?ext_bloat", "0.37mm")
		set_arg(P, "?int_bloat", "0.37mm")
		set_arg(P, "?pad_thickness", "0.3mm")
		set_arg(P, "?silkmark", "dot")
		set_arg(P, "?line_thickness", "0.1mm")
		set_arg(P, "?cpm_width", "1mm")
		set_arg(P, "?cpm_height", "1mm")
		set_arg(P, "?cpm_nx", "2")
		set_arg(P, "?cpm_ny", "2")
	}

	reqs = "nx,ny"
	
	if (pre_args != "")
		reqs=""

	if ((post_args != "") && (!(post_args ~ "^,")))
		post_args = "," post_args

	if ((pre_args != "") && (!(pre_args ~ ",$")))
		pre_args = pre_args ","

	proc_args(P, pre_args "nx,ny,x_spacing,y_spacing,pad_spacing,ext_bloat,int_bloat,width,height,cpad_width,cpad_height,cpad_auto,cpm_nx,cpm_ny,cpm_width,cpm_height,cpad_mask,rpad_round,bodysilk,pinoffs,silkmark" post_args, reqs)

	nx = int(P["nx"])
	ny = int(P["ny"])

	if (P["ny"] == "")
		ny = nx

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
	pinoffs = int(P["pinoffs"])

	if (x_spacing == "")
		x_spacing = (nx+hook_spc_conv) * pad_spacing
	if (y_spacing == "")
		y_spacing = (ny+hook_spc_conv) * pad_spacing

	cpad_width=parse_dim(P["cpad_width"])
	cpad_height=parse_dim(P["cpad_height"])

	if (P["cpad_mask"] == "")
		cpad_mask = (cpad_width > cpad_height ? cpad_width : cpad_height) +  either(parse_dim(P["pad_mask"]), DEFAULT["pad_mask"])
	else
		cpad_mask=parse_dim(P["cpad_mask"])


	if (tobool(P["cpad_auto"]) || hook_cpad_auto) {
		if (cpad_width == "")
			cpad_width = (x_spacing*0.85 - int_bloat*2 - pt) * hook_cpad_mult
		if (cpad_height == "")
			cpad_height = (y_spacing*0.85 - int_bloat*2 - pt) * hook_cpad_mult
	}


	if (width == "")
		width = x_spacing
	if (height == "")
		height = y_spacing

	pinmax=(nx+ny)*2

	if (!tobool(P["rpad_round"]))
		rpad_round = "square"
	else
		rpad_round = ""


	cpm_width = parse_dim(P["cpm_width"])
	cpm_height = parse_dim(P["cpm_height"])
	cpm_nx = int(P["cpm_nx"])
	cpm_ny = int(P["cpm_ny"])
}

function pinnum(num)
{
	return ((num-1) + pinoffs) % (pinmax)+1
}

BEGIN {
	qf_globals()


	element_begin("", "U1", 2*nx + 2*ny   ,0,0, -width/2 - mm(1), -height/2 - mm(2))

	cx = (nx+1)/2
	cy = (ny+1)/2

	for(n = 1; n <= ny; n++) {
		y = (-cy + n) * pad_spacing
		x1 = -x_spacing/2
		x2 = x_spacing/2
		element_pad(x1-ext_bloat, y, x1+int_bloat, y, pad_width, pinnum(n), rpad_round)
		element_pad(x2-int_bloat, y, x2+ext_bloat, y, pad_width, pinnum(nx+2*ny-n+1), rpad_round)
		if (n == 1)
			y1 = y
		if (n == 2)
			dimension(x1, y1, x1, y, (ext_bloat * -3), "pad_spacing")
	}

	dimension(x1, y-pt/2, x1, y+pt/2, (ext_bloat * -3), "pad_thickness")

	for(n = 1; n <= nx; n++) {
		x = (-cx + n) * pad_spacing
		y1 = -y_spacing/2
		y2 = y_spacing/2
		element_pad(x, y1-ext_bloat, x, y1+int_bloat, pad_width, pinnum(nx*2+ny*2-n+1), rpad_round)
		element_pad(x, y2-int_bloat, x, y2+ext_bloat, pad_width, pinnum(n+ny), rpad_round)
	}


	if ((cpad_width != "") && (cpad_height != "")) {
#		center pad paste matrix
		if ((cpm_nx > 0) && (cpm_ny > 0)) {
			ox = (cpad_width - (cpm_nx*cpm_width)) / (cpm_nx - 1)
			oy = (cpad_height - (cpm_ny*cpm_height)) / (cpm_ny - 1)
			element_pad_matrix(-cpad_width/2, -cpad_height/2,   cpm_nx,cpm_ny,  cpm_width,cpm_height,  ox+cpm_width,oy+cpm_height,   2*nx+2*ny+1, "square,nopaste")
		}

#		center pad
		tmp = DEFAULT["pad_mask"]
		DEFAULT["pad_mask"] = cpad_mask
		element_pad_rectangle(-cpad_width/2, -cpad_height/2, +cpad_width/2, +cpad_height/2, 2*nx+2*ny+1, "square,nopaste")
		DEFAULT["pad_mask"] = tmp
		dimension(-cpad_width/2, -cpad_height/2, +cpad_width/2, -cpad_height/2, "@0;" (height * -0.6-ext_bloat), "cpad_width")
		dimension(cpad_width/2, -cpad_height/2, cpad_width/2, +cpad_height/2, "@" (width * 0.8+ext_bloat) ";0", "cpad_height")
	}

	wx = (width  - nx * pad_spacing) / 3.5
	wy = (height - ny * pad_spacing) / 3.5

	bodysilk = P["bodysilk"]
	if ((bodysilk == "corners") || (bodysilk == "")) {
		element_rectangle_corners(-width/2, -height/2, width/2, height/2, wx, wy)
		silkmark(P["silkmark"], -width/2 - wx/2, -height/2+wy*1.5, (wx+wy)/4)
	}
	else if (bodysilk == "full") {
		element_rectangle(-width/2, -height/2, width/2, height/2)
		silkmark(P["silkmark"], -width/2 + wx*3, -height/2+wy*3, (wx+wy)/2)
	}
	else if (bodysilk == "plcc") {
		r = (width+height)/10
		element_rectangle(-width/2, -height/2, width/2, height/2, "arc,NE,SW,SE", r)
		element_line(-width/2, -height/2+r, width/2, -height/2+r)
		silkmark(P["silkmark"], 0, -height*0.2, height/40)
	}
	else if (bodysilk != "none")
		error("invalid bodysilk parameter")

	dimension(-width/2, -height/2, +width/2, -height/2, "@0;" height*-0.8-ext_bloat,       "width")
	dimension(+width/2, -height/2, +width/2, +height/2, "@" (width * 1+ext_bloat) ";0",  "height")

	dimension(-x_spacing/2, -height/2, +x_spacing/2, -height/2, "@0;" height*-1-ext_bloat,       "x_spacing")
	dimension(+width/2, -y_spacing/2, +width/2, +y_spacing/2, "@" (width * 1.2+ext_bloat) ";0",  "y_spacing")



	element_end()
}
