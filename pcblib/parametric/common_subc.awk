#@@param:pin_ringdia pin's copper ring (annulus) outer diameter (in mil by default, mm suffix can be used)
#@@optional:pin_ringdia
#@@dim:pin_ringdia

#@@param:pin_clearance pin's copper clearance diameter (in mil by default, mm suffix can be used)
#@@optional:pin_clearance
#@@dim:pin_clearance

#@@param:pin_mask pin's solder mask diameter (in mil by default, mm suffix can be used)
#@@optional:pin_mask
#@@dim:pin_mask

#@@param:pin_mask_offs how much bigger (+) or smaller (-) the mask opening should be compared to copper (in mil by default, mm suffix can be used)
#@@optional:pin_mask_offs
#@@dim:pin_mask_offs

#@@param:pin_mask_ratio pin mask opening should be copper size * this ratio (numbers larger than 1 mean opening with a gap, 1.00 means exactly as big as the copper shape)
#@@optional:pin_mask_ratio
#@@dim:pin_mask_ratio

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

#@@param:pad_mask_offs how much bigger (+) or smaller (-) the mask opening should be compared to copper (in mil by default, mm suffix can be used)
#@@optional:pad_mask_offs
#@@dim:pad_mask_offs

#@@param:pad_mask_ratio pad mask opening should be copper size * this ratio (numbers larger than 1 mean opening with a gap, 1.00 means exactly as big as the copper shape)
#@@optional:pad_mask_ratio
#@@dim:pad_mask_ratio

#@@param:pad_paste pad's paste line thickness (in mil by default, mm suffix can be used)
#@@optional:pad_paste
#@@dim:pad_paste

#@@param:pad_paste_offs how much bigger (+) or smaller (-) the paste should be compared to copper (in mil by default, mm suffix can be used)
#@@optional:pad_paste_offs
#@@dim:pad_paste_offs

#@@param:pad_paste_ratio pad paste should be copper size * this ratio (numbers smaller than 1 mean smaller paste, 1.00 means exactly as big as the copper shape)
#@@optional:pad_paste_ratio
#@@dim:pad_paste_ratio

#@@param:line_thickness silk line thickness (in mil by default, mm suffix can be used)
#@@optional:line_thickness
#@@dim:line_thickness

BEGIN {
	q="\""

	DEFAULT["pin_ringdia"] = mil(80)
	DEFAULT["pin_ringdia", "dim"] = 1

	DEFAULT["pin_clearance"] = mil(50)
	DEFAULT["pin_clearance", "dim"] = 1

	DEFAULT["pin_mask"] = mil(86)
	DEFAULT["pin_mask", "dim"] = 1
	DEFAULT["pin_mask_offs"] = "" # use copper size
	DEFAULT["pin_mask_offs", "dim"] = 1
	DEFAULT["pin_mask_ratio"] = "" # use copper size

	DEFAULT["pin_drill"] = mm(1)
	DEFAULT["pin_drill", "dim"] = 1

	DEFAULT["line_thickness"] = mil(10)
	DEFAULT["line_thickness", "dim"] = 1

	DEFAULT["pad_thickness"] = mil(20)
	DEFAULT["pad_thickness", "dim"] = 1
	DEFAULT["pad_clearance"] = mil(10)
	DEFAULT["pad_clearance", "dim"] = 1
	DEFAULT["pad_mask"] = mil(30)
	DEFAULT["pad_mask", "dim"] = 1
	DEFAULT["pad_mask_offs"] = "" # use copper size
	DEFAULT["pad_mask_offs", "dim"] = 1
	DEFAULT["pad_mask_ratio"] = "" # use copper size
	DEFAULT["pad_paste"] = "" # use copper size
	DEFAULT["pad_paste", "dim"] = 1
	DEFAULT["pad_paste_offs"] = "" # use copper size
	DEFAULT["pad_paste_offs", "dim"] = 1
	DEFAULT["pad_paste_ratio"] = "" # use copper size

	DEFAULT["pin_flags"] = "__auto"

	s_default=1
	s_weak=2
	s_explicit=3
	
	offs_x = 0
	offs_y = 0
	objid = 1
	proto_next_id = 0

	pi=3.141592654

	NL = "\n"

# minuid
	for(n = 32; n < 127; n++)
		ORD[sprintf("%c", n)] = n
	BASE64 = "ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
}

function minuid_add(SUM, s    ,n,v,c1,c2)
{
	v = length(s)
	for(n = 1; n <= v; n++)
		SUM[(n-1) % 20] += ORD[substr(s, n, 1)]-32
}

function minuid_str(SUM, s    ,n)
{
	s = "Prm/"
	for(n = 0; n < 20; n++)
		s = s substr(BASE64, SUM[n] % 64, 1)
	return s
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

function lht_str(s)
{
	if (s ~ "[^A-Za-z0-9 _-]") {
		gsub("}", "\\}", s)
		return "{" s "}"
	}
	return s
}

function unit(coord)
{
	if (coord == "")
		coord = 0
	if (base_unit_mm)
		return coord "mm"
	return coord "mil"
}

function subc_text(layer, x, y, str, scale, rot, flags, attributes      ,s)
{

	s = s "      ha:text." ++objid "{" NL
	s = s "       scale = " either(scale, 100) NL
	if (attributes != "")
	s = s "       ha:attributes {" attributes "}" NL
	s = s "       x = " unit(x) NL
	s = s "       y = " unit(y) NL
	s = s "       rot = " either(rot, 0) NL
	s = s "       string = %a.parent.refdes%" NL
	s = s "       fid = 0" NL
	s = s "       ha:flags {" flags "}" NL
	s = s "      }" NL

	LAYER[layer] = LAYER[layer] NL s
}

function subc_line(layer, x1, y1, x2, y2, thick, clr, flags, attributes      ,s)
{
	s = s "      ha:line." ++objid " {" NL
	s = s "       x1 = " unit(x1) NL
	s = s "       y1 = " unit(y1) NL
	s = s "       x2 = " unit(x2) NL
	s = s "       y2 = " unit(y2) NL
	s = s "       thickness = " unit(either(thick, DEFAULT["line_thickness"])) NL
	s = s "       clearance = " unit(either(clr, 0)) NL
	if (attributes != "")
	s = s "       ha:attributes {" attributes "}" NL
	s = s "       ha:flags {" flags "}" NL
	s = s "      }" NL

	LAYER[layer] = LAYER[layer] NL s
}

function subc_arc(layer, cx, cy, r, a_start, a_delta, thick, clr, flags, attributes      ,s)
{
	s = s "      ha:arc." ++objid " {" NL
	s = s "       x = " unit(cx) NL
	s = s "       y = " unit(cy) NL
	s = s "       astart = " a_start NL
	s = s "       adelta = " a_delta NL
	s = s "       thickness = " unit(either(thick, DEFAULT["line_thickness"])) NL
	s = s "       clearance = " unit(either(clr, 0)) NL
	s = s "       width = " unit(r) NL
	s = s "       height = " unit(r) NL
	if (attributes != "")
	s = s "       ha:attributes {" attributes "}" NL
	s = s "       ha:flags {" flags "}" NL
	s = s "      }" NL

	LAYER[layer] = LAYER[layer] NL s
}

function subc_rect(layer, x1, y1, x2, y2, clearance, flags, attributes     ,s)
{
	w = w/2
	h = h/2
	s = s "          ha:polygon." ++objid " {" NL
	s = s "           clearance=" unit(clearance) NL
	s = s "           li:geometry {" NL
	s = s "             ta:contour {" NL
	s = s "              { " unit(x1) "; " unit(y1) " }" NL
	s = s "              { " unit(x2) "; " unit(y1) " }" NL
	s = s "              { " unit(x2) "; " unit(y2) " }" NL
	s = s "              { " unit(x1) "; " unit(y2) " }" NL
	s = s "             }" NL
	s = s "           }" NL
	s = s "           ha:attributes {" attributes "}" NL
	s = s "           ha:flags {" flags "}" NL
	s = s "          }" NL

	LAYER[layer] = LAYER[layer] NL s
}

# start generating a subcircuit
function subc_begin(footprint, refdes, refdes_x, refdes_y, refdes_dir)
{
	print "li:pcb-rnd-subcircuit-v6 {"
	print " ha:subc." ++objid "{"
	print "  ha:attributes {"
	print "   footprint = " lht_str(footprint)
	if (refdes != "")
		print "   refdes = " lht_str(refdes)
	print "  }"


	subc_text("top-silk", refdes_x, refdes_y, "%a.parent.refdes%", 100, text_dir, "dyntext = 1;floater=1;")
	LAYER_TYPE["subc-aux"] = "top-misc-virtual"
	subc_line("subc-aux", -offs_x, -offs_y, -offs_x + mm(1), -offs_y, mm(0.1), 0, "", "subc-role = x");
	subc_line("subc-aux", -offs_x, -offs_y, -offs_x, -offs_y + mm(1), mm(0.1), 0, "", "subc-role = y");
	subc_line("subc-aux", -offs_x, -offs_y, -offs_x, -offs_y, mm(0.1), 0, "", "subc-role = origin");
}

# generate subcircuit footers
function subc_end(     layer,n,v,L,lt,UID)
{
	minuid_add(UID, tolower(gen))
	for(n in P) {
		minuid_add(UID, tolower(n))
		minuid_add(UID, "          " P[n])
	}
	print "  ha:data {"
	print "   li:padstack_prototypes {"
	for(n = 0; n < proto_next_id; n++) {
		if (PROTO_COMMENT[n] != "")
			print PROTO_COMMENT[n]
		print "    ha:ps_proto_v6." n " {"
		print PROTO[n]
		print "    }"
	}
	print "   }"

# global objects (padstack refs)
	print "   li:objects {"
	print globals
	print "   }"

# layers and layer objects
	print "   li:layers {"
	for(layer in LAYER) {
		lt = either(LAYER_TYPE[layer], layer)
		v = split(lt, L, "-")
		print "    ha:" layer " {"
		print "     lid = 0"
		print "     ha:type {"
		for(n = 1; n <= v; n++)
			print "      " L[n] " = 1"
		print "     }"
		print "     li:objects {"
		print LAYER[layer]
		print "     }"
		print "     ha:combining {"
		print "     }"
		print "    }"
	}
	print "   }"
	print "  }"
	print "  uid = " minuid_str(UID)
	print " }"
	print "}"
}

function subc_proto_alloc()
{
	return proto_next_id++
}

function subc_pstk_add_hole(proto, dia, plated,     htop, hbottom    ,s)
{
	s = s "     hdia = " unit(dia) NL
	s = s "     hplated = " int(plated) NL
	s = s "     htop = " int(htop) NL
	s = s "     hbottom = " int(hbottom) NL
	PROTO[proto] = PROTO[proto] s
	PROTO_HOLE[proto] = dia
}

function subc_pstk_no_hole(proto   ,s)
{
	s = s "     hdia = 0; hplated = 0; htop = 0; hbottom = 0" NL
	PROTO[proto] = PROTO[proto] s
}

function subc_pstk_shape_layer(layer     ,s,L,v,n)
{
	v = split(layer, L, "-")
	s = s "       ha:layer_mask {" NL
	for(n = 1; n <= v; n++)
		s = s "        " L[n] " = 1" NL
	s = s "       }" NL
	s = s "       ha:combining {" NL
	if (layer ~ "mask")
	s = s "        sub = 1"  NL
	if ((layer ~ "mask") || (layer ~ "paste"))
	s = s "        auto = 1"  NL
	s = s "       }" NL
	return s
}

function subc_pstk_add_shape_circ(proto, layer, x, y, dia    ,s)
{
	s = s "      ha:ps_shape_v4 {" NL
	s = s "       clearance = 0" NL
	s = s "       ha:ps_circ {" NL
	s = s "        x = " unit(x) NL
	s = s "        y = " unit(y) NL
	s = s "        dia = " unit(dia) NL
	s = s "       }" NL
	s = s subc_pstk_shape_layer(layer)
	s = s "      }" NL
	PROTO[proto] = PROTO[proto] s
}

function subc_pstk_add_shape_square(proto, layer, x, y, sx, sy    ,s)
{
	sx = sx / 2
	sy = sy / 2
	s = s "      ha:ps_shape_v4 {" NL
	s = s "       clearance = 0" NL
	s = s "       li:ps_poly {" NL
	s = s "        " unit(x - sx) ";" unit(y - sy) ";  " unit(x + sx) ";" unit(y - sy) ";" NL
	s = s "        " unit(x + sx) ";" unit(y + sy) ";  " unit(x - sx) ";" unit(y + sy) ";" NL
	s = s "       }" NL
	s = s subc_pstk_shape_layer(layer)
	s = s "      }" NL
	PROTO[proto] = PROTO[proto] s
}

function subc_pstk_add_shape_square_corners(proto, layer, x1, y1, x2, y2    ,s)
{
	sx = sx / 2
	sy = sy / 2
	s = s "      ha:ps_shape_v4 {" NL
	s = s "       clearance = 0" NL
	s = s "       li:ps_poly {" NL
	s = s "        " unit(x1) ";" unit(y1) ";  " unit(x2) ";" unit(y1) ";" NL
	s = s "        " unit(x2) ";" unit(y2) ";  " unit(x1) ";" unit(y2) ";" NL
	s = s "       }" NL
	s = s subc_pstk_shape_layer(layer)
	s = s "      }" NL
	PROTO[proto] = PROTO[proto] s
}

function subc_pstk_add_shape_line(proto, layer, x1, y1, x2, y2, thick    ,s)
{
	s = s "      ha:ps_shape_v4 {" NL
	s = s "       clearance = 0" NL
	s = s "       ha:ps_line {" NL
	s = s "        x1=" unit(x1) "; y1=" unit(y1) "; x2=" unit(x2) "; y2=" unit(y2) ";" NL
	s = s "        thickness=" unit(thick) "; square=0" NL
	s = s "       }" NL
	s = s subc_pstk_shape_layer(layer)
	s = s "      }" NL
	PROTO[proto] = PROTO[proto] s
}

function subc_proto_create_pin_round(drill_dia, ring_dia, mask_dia      ,proto)
{
	proto = subc_proto_alloc()
	subc_pstk_add_hole(proto, either(drill_dia, DEFAULT["pin_drill"]), 1)

	PROTO_COMMENT[proto] = "# Round plated through hole " unit(ring_dia) "/" unit(drill_dia)
	PROTO[proto] = PROTO[proto] "     li:shape {" NL

	ring_dia = either(ring_dia, DEFAULT["pin_ringdia"])
	subc_pstk_add_shape_circ(proto, "top-copper", x, y, ring_dia)
	subc_pstk_add_shape_circ(proto, "intern-copper", x, y, ring_dia)
	subc_pstk_add_shape_circ(proto, "bottom-copper", x, y, ring_dia)

	mask_dia = pin_mask(ring_dia, mask_dia)
	subc_pstk_add_shape_circ(proto, "top-mask", x, y, mask_dia)
	subc_pstk_add_shape_circ(proto, "bottom-mask", x, y, mask_dia)

	PROTO[proto] = PROTO[proto] "     }" NL
	return proto
}

function subc_proto_create_pin_square(drill_dia, ring_span, mask_span     ,proto)
{
	proto = subc_proto_alloc()
	subc_pstk_add_hole(proto, either(drill_dia, DEFAULT["pin_drill"]), 1)

	PROTO_COMMENT[proto] = "# Square plated through hole " unit(ring_dia) "/" unit(drill_dia)
	PROTO[proto] = PROTO[proto] "     li:shape {" NL

	ring_span = either(ring_span, DEFAULT["pin_ringdia"])
	subc_pstk_add_shape_square(proto, "top-copper", x, y, ring_span, ring_span)
	subc_pstk_add_shape_square(proto, "intern-copper", x, y, ring_span, ring_span)
	subc_pstk_add_shape_square(proto, "bottom-copper", x, y, ring_span, ring_span)

	mask_span = pin_mask(ring_span, mask_dia)
	subc_pstk_add_shape_square(proto, "top-mask", x, y, mask_span, mask_span)
	subc_pstk_add_shape_square(proto, "bottom-mask", x, y, mask_span, mask_span)
	PROTO[proto] = PROTO[proto] "     }" NL

	return proto
}

function paste_or_mask_abs(copper, absval, offsval, ratio, prefix)
{
	if (absval != "")
		return absval
	if (offsval != 0)
		return copper+offsval
	if (ratio != 0)
		return copper*ratio
	if ((DEFAULT[prefix] != "") && (DEFAULT[prefix] != "-"))
		return DEFAULT[prefix]
	if (DEFAULT[prefix "_offs"] != "")
		return copper+DEFAULT[prefix "_offs"]*2
	if (DEFAULT[prefix "_ratio"] != "")
		return copper*DEFAULT[prefix "_ratio"]
	return copper
}

function pad_paste(copper, absval, offsval, ratio)
{
	return paste_or_mask_abs(copper, absval, offsval, ratio, "pad_paste")
}

function pad_mask(copper, absval, offsval, ratio)
{
	return paste_or_mask_abs(copper, absval, offsval, ratio, "pad_mask")
}

function pin_mask(copper, absval, offsval, ratio)
{
	return paste_or_mask_abs(copper, absval, offsval, ratio, "pin_mask")
}

function pad_paste_offs(offsval            ,copper)
{
	copper = DEFAULT["pad_thickness"]
	return paste_or_mask_abs(copper, "", offsval, "", "pad_paste") - copper
}

function pad_mask_offs(offsval            ,copper)
{
	copper = DEFAULT["pad_thickness"]
	return paste_or_mask_abs(copper, "", offsval, "", "pad_mask") - copper
}


function subc_proto_create_pad_sqline(x1, x2, thick, mask, paste   ,proto,m,p)
{
	proto = subc_proto_alloc()

	thick = either(thick, DEFAULT["pad_thickness"])

	subc_pstk_no_hole(proto)

	PROTO_COMMENT[proto] = "# Square smd pad " x2-x1 " * " thick
	PROTO[proto] = PROTO[proto] "     li:shape {" NL

	subc_pstk_add_shape_square_corners(proto, "top-copper", x1-thick/2, -thick/2, x2+thick/2, thick/2)

	m = (pad_mask(thick, mask)-thick)/2
	subc_pstk_add_shape_square_corners(proto, "top-mask", x1-thick/2-m, -thick/2-m, x2+thick/2+m, thick/2+m)

	p = (pad_paste(thick, paste)-thick)/2
	subc_pstk_add_shape_square_corners(proto, "top-paste", x1-thick/2-p, -thick/2-p, x2+thick/2+p, thick/2+p)

	PROTO[proto] = PROTO[proto] "     }" NL

	return proto
}

function subc_proto_create_pad_line(x1, x2, thick, mask, paste   ,proto,m,p)
{
	proto = subc_proto_alloc()

	thick = either(thick, parse_dim(DEFAULT["pad_thickness"]))

	subc_pstk_no_hole(proto)

	PROTO_COMMENT[proto] = "# Square smd pad " x2-x1 " * " thick
	PROTO[proto] = PROTO[proto] "     li:shape {" NL

	subc_pstk_add_shape_line(proto, "top-copper", x1, 0, x2, 0, thick)
	subc_pstk_add_shape_line(proto, "top-mask", x1, 0, x2, 0, pad_mask(thick, mask))
	subc_pstk_add_shape_line(proto, "top-paste", x1, 0, x2, 0, pad_paste(thick, paste))

	PROTO[proto] = PROTO[proto] "     }" NL

	return proto
}

function subc_proto_create_pad_rect(w, h, mask_offs, paste_offs   ,proto,m,p)
{
	proto = subc_proto_alloc()

	subc_pstk_no_hole(proto)

	PROTO_COMMENT[proto] = "# Square smd pad " w " * " h
	PROTO[proto] = PROTO[proto] "     li:shape {" NL

	w = w/2
	h = h/2

	subc_pstk_add_shape_square_corners(proto, "top-copper", -w, -h, +w, +h)

	if (mask_offs != "none") {
		m = pad_paste_offs(w, mask_offs) / 2
		subc_pstk_add_shape_square_corners(proto, "top-mask", -w-m, -h-m, +w+m, +h+m)
	}

	if (paste_offs != "none") {
		p = pad_paste_offs(w, paste_offs) / 2
		subc_pstk_add_shape_square_corners(proto, "top-paste", -w-p, -h-p, +w+p, +h+p)
	}

	PROTO[proto] = PROTO[proto] "     }" NL

	return proto
}


function subc_proto_create_pad_circle(dia, mask_dia, paste_dia    ,proto)
{
	proto = subc_proto_alloc()
	subc_pstk_no_hole(proto)

	dia = either(dia, DEFAULT["pad_dia"])

	PROTO_COMMENT[proto] = "# Circular smd pad " unit(dia)
	PROTO[proto] = PROTO[proto] "     li:shape {" NL

	subc_pstk_add_shape_circ(proto, "top-copper", 0, 0, dia)

	mask_dia = either(mask_dia, DEFAULT["pad_mask_dia"])
	subc_pstk_add_shape_circ(proto, "top-mask", 0, 0, mask_dia)

	paste_dia = either(mask_dia, DEFAULT["pad_paste_dia"])
	subc_pstk_add_shape_circ(proto, "top-mask", 0, 0, paste_dia)

	PROTO[proto] = PROTO[proto] "     }" NL
	return proto
}

# generate a padstack reference
function subc_pstk(proto, x, y, rot, termid, name, clearance,      s)
{
	if (termid == "")
		termid = ++pin_number

	s = s "    ha:padstack_ref." ++objid " {" NL
	s = s "     proto = " proto NL
	s = s "     x = " unit(x) NL
	s = s "     y = " unit(y) NL
	s = s "     rot = " rot+0 NL
	s = s "     smirror = 0; xmirror = 0" NL
	s = s "     clearance = " unit(either(clearance, (PROTO_HOLE[proto] > 0 ? DEFAULT["pin_clearance"] : DEFAULT["pad_clearance"]))/2) NL
	s = s "     ha:attributes {" NL
	s = s "      term = " termid NL
	if (name != "")
		s = s "      name = 1" NL
	s = s "     }" NL
	s = s "     li:thermal { }" NL
	s = s "     ha:flags { clearline = 1; }" NL
	s = s "    }" NL

	globals = globals  NL s
}

# draw element pad
function subc_pad(x1, y1, x2, y2, thickness,   number, flags,   clearance, mask, name)
{
	print "	Pad[", x1, y1, x2, y2, int(either(thickness, DEFAULT["pad_thickness"])),
		int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
		q name q, q number q, q flags q "]"
}

# draw element pad - no thickness, but exact corner coordinates given
function subc_pad_rectangle(x1, y1, x2, y2,   number, flags,   clearance, mask, name,     th,dx,dy,cx,cy)
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

		print "	Pad[", x1+th/2, cy, x2-th/2, cy, th,
			int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
			q name q, q number q, q flags q "]"
	}
	else {
		th = dx
		cx = (x1+x2)/2

		print "	Pad[", cx, y1+th/2, cx, y2-th/2, th,
			int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
			q name q, q number q, q flags q "]"
	}
}

# draw element pad circle
function subc_pad_circle(x1, y1, radius,   number,  clearance, mask, name)
{
	print "	Pad[", x1, y1, x1, y1, int(either(radius, DEFAULT["pad_thickness"])),
		int(either(clearance, DEFAULT["pad_clearance"])), int(either(mask, DEFAULT["pad_mask"])),
		q name q, q number q, q "" q "]"
}

function subc_arrow(layer, x1, y1, x2, y2,  asize,  thickness   ,vx,vy,nx,ny,len,xb,yb)
{
	subc_line(layer, x1, y1, x2,y2, thickness)

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
#		subc_line(layer, x2, y2, xb + 1000, yb + 1000)
		subc_line(layer, x2, y2, xb + nx*asize/2, yb + ny*asize/2)
		subc_line(layer, x2, y2, xb - nx*asize/2, yb - ny*asize/2)
		subc_line(layer, xb - nx*asize/2, yb - ny*asize/2, xb + nx*asize/2, yb + ny*asize/2)
	}
}

# draw a rectangle of silk lines 
# omit sides as requested in omit
# if r is non-zero, round corners - omit applies as NW, NW, SW, SE
# if omit includes "arc", corners are "rounded" with lines
function subc_rectangle(layer, x1, y1, x2, y2,    omit,  r, thickness   ,tmp,r1,r2)
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
		subc_line(layer, x1, y1+r1, x1, y2-r2, thickness)
	}
	if (!(omit ~ "top")) {
		r1 = (omit ~ "NW") ? 0 : r
		r2 = (omit ~ "NE") ? 0 : r
		subc_line(layer, x1+r1, y1, x2-r2, y1, thickness)
	}
	if (!(omit ~ "bottom")) {
		r1 = (omit ~ "SE") ? 0 : r
		r2 = (omit ~ "SW") ? 0 : r
		subc_line(layer, x2-r1, y2, x1+r2, y2, thickness)
	}
	if (!(omit ~ "right")) {
		r1 = (omit ~ "SE") ? 0 : r
		r2 = (omit ~ "NE") ? 0 : r
		subc_line(layer, x2, y2-r1, x2, y1+r2, thickness)
	}

	if (r > 0) {
		if (omit ~ "arc") {
			if (!(omit ~ "NW"))
				subc_line(layer, x1, y1+r, x1+r, y1)
			if (!(omit ~ "SW"))
				subc_line(layer, x1, y2-r, x1+r, y2)
			if (!(omit ~ "NE"))
				subc_line(layer, x2, y1+r, x2-r, y1)
			if (!(omit ~ "SE"))
				subc_line(layer, x2, y2-r, x2-r, y2)
		}
		else {
			if (!(omit ~ "NW"))
				subc_arc(layer, x1+r, y1+r, r, 270, 90)
			if (!(omit ~ "SW"))
				subc_arc(layer, x1+r, y2-r, r, 0, 90)
			if (!(omit ~ "NE"))
				subc_arc(layer, x2-r, y1+r, r, 180, 90)
			if (!(omit ~ "SE"))
				subc_arc(layer, x2-r, y2-r, r, 90, 90)
		}
	}
}

# draw a rectangle corners of silk lines, wx and wy long in x and y directions
# omit sides as requested in omit: NW, NW, SW, SE
# corners are always sharp
function subc_rectangle_corners(layer, x1, y1, x2, y2, wx, wy,    omit,  thickness   ,tmp)
{
	if (!(omit ~ "NW")) {
		subc_line(layer, x1, y1,     x1+wx, y1,   thickness)
		subc_line(layer, x1, y1,     x1, y1+wy,   thickness)
	}
	if (!(omit ~ "NE")) {
		subc_line(layer, x2-wx, y1,  x2, y1,    thickness)
		subc_line(layer, x2, y1,     x2, y1+wy, thickness)
	}
	if (!(omit ~ "SW")) {
		subc_line(layer, x1, y2,     x1+wx, y2, thickness)
		subc_line(layer, x1, y2-wy,  x1, y2,    thickness)
	}
	if (!(omit ~ "SE")) {
		subc_line(layer, x2-wx, y2,  x2, y2,   thickness)
		subc_line(layer, x2, y2-wy,  x2, y2,   thickness)
	}
}

# convert coord given in mils to footprint units
function mil(coord)
{
	if (base_unit_mm)
		return coord / 39.3701
	else
		return coord
}

# reverse mil(): converts footprint units back to mil
function rev_mil(coord)
{
	if (base_unit_mm)
		return coord * 39.3701
	else
		return coord
}


# convert coord given in mm to footprint units
function mm(coord)
{
	if (base_unit_mm)
		return coord
	else
		return coord * 39.3701
}

# reverse mm(): converts footprint units back to mm
function rev_mm(coord)
{
	if (base_unit_mm)
		return coord
	else
		return coord / 39.3701
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
function dip_outline(layer, x1, y1, x2, y2, arcr   ,xhalf)
{
	if (xhalf == "")
		xhalf=(x1+x2)/2

	subc_rectangle(layer, x1, y1, x2, y2, "top")
	subc_line(layer, x1, y1, xhalf-arcr, y1)
	subc_line(layer, xhalf+arcr, y1, x2, y1)

	subc_arc(layer, xhalf, y1,  arcr,  0, 180)
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
			subc_line("top-silk", x+half, y,  x, y+half)
		}
		else if (S[n] == "square") {
			subc_line("top-silk", x, y+step,  x+2*half, y+step)
			subc_line("top-silk", x+step, y,  x+2*half, y+step)
		}
		else if ((S[n] == "external") || (S[n] == "externalx")) {
			subc_line("top-silk", x, y+half,         x-step+half, y+half/2)
			subc_line("top-silk", x, y+half,         x-step+half, y+half*1.5)
			subc_line("top-silk", x-step+half, y+half/2,         x-step+half, y+half*1.5)
		}
		else if (S[n] == "externaly") {
			subc_line("top-silk", x+half, y,         x-half/2+half, y-step+half)
			subc_line("top-silk", x+half, y,         x+half/2+half, y-step+half)
			subc_line("top-silk", x-half/2+half, y-step+half,  x+half/2+half, y-step+half)
		}
		else if (S[n] == "external45") {
			subc_line("top-silk", x, y,       x-half, y-half/3)
			subc_line("top-silk", x, y,       x-half/3, y-half)
			subc_line("top-silk", x-half, y-half/3, x-half/3, y-half)
		}
		else if (S[n] == "arc") {
			subc_arc("top-silk", x, y, step/2, 180, 270)
		}
		else if (S[n] == "circle") {
			subc_arc("top-silk", x, y, step/2, 0, 360)
		}
		else if (S[n] == "dot") {
			subc_arc("top-silk", x-step/2, y-step/2, step/4, 0, 360)
		}
		else if ((S[n] != "none") && (S[n] != "")) {
			error("invalid silkmark parameter: " S[n])
		}
	}
}

function center_pad_init()
{
	cpm_width = parse_dim(P["cpm_width"])
	cpm_height = parse_dim(P["cpm_height"])
	cpm_nx = int(P["cpm_nx"])
	cpm_ny = int(P["cpm_ny"])
}

function center_pad(cpadid, cpx, cpy)
{
	if ((cpad_width != "") && (cpad_height != "")) {
#		center pad paste matrix
		if ((cpm_nx > 0) && (cpm_ny > 0)) {
			ox = (cpad_width - (cpm_nx*cpm_width)) / (cpm_nx - 1)
			oy = (cpad_height - (cpm_ny*cpm_height)) / (cpm_ny - 1)
			paste_matrix(cpx-cpad_width/2, xpy-cpad_height/2,   cpm_nx,cpm_ny,  cpm_width,cpm_height,
			  ox+cpm_width,oy+cpm_height, "", "termid=" cpadid ";", 0)
		}

#		center pad
		cpad_proto = subc_proto_create_pad_rect(cpad_width, cpad_height, cpad_mask == "" ? 0 : cpad_mask, "none")
		subc_pstk(cpad_proto, cpx, cpy, 0, cpadid)
		dimension(cpx-cpad_width/2, cpy-cpad_height/2, cpx+cpad_width/2, cpy-cpad_height/2, "@0;" (height * -0.6-ext_bloat), "cpad_width")
		dimension(cpx+cpad_width/2, cpy-cpad_height/2, cpx+cpad_width/2, cpy+cpad_height/2, "@" (width * 0.8+ext_bloat) ";0", "cpad_height")
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
	print "#dimension", x1, y1, x2, y2, dist, name, value
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
	print "#@@info-generator pcblib common_subc.awk"
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
