BEGIN {
	PkgLoad("pcb-rnd-gpmi/actions", 0);
	PkgLoad("pcb-rnd-gpmi/dialogs", 0);
	PkgLoad("pcb-rnd-gpmi/layout", 0);
	pi=3.141592654
}

# the clock coordinate system
function clock_cos(a)
{
	return cos((a-15)*6/180*pi)
}
function clock_sin(a)
{
	return sin((a-15)*6/180*pi)
}

# draw the labels
function draw_dial(ui, cx, cy, r    ,flg,w,cl,a,l,x1,y1,x2,y2)
{
	flg = "FL_NONE"
	w = int(0.5 * mm)
	cl = int(1 * mm)

	for(a = 0; a < 60; a++) {
		tw = w
		if (a == 0)
			l = 4*mm
		else if (a % 5 == 0)
			l = 2*mm
		else {
			l = 1*mm
			tw = w / 2
		}

		x1 = int(cx + clock_cos(a) * r)
		y1 = int(cy + clock_sin(a) * r)
		x2 = int(cx + clock_cos(a) * (r-l))
		y2 = int(cy + clock_sin(a) * (r-l))
		layout_create_line("", ui, x1, y1, x2, y2, tw, cl, flg)


		if ((a % 5) == 0) {
			x2 = int(cx + clock_cos(a) * (r-6*mm)) - 0.25*mm
			y2 = int(cy + clock_sin(a) * (r-6*mm))
			layout_create_text("", ui, x2, y2, 0, 100, ((a == 0) ? "12" : a/5), flg)
		}
	}

	hlen[0] = 0.95
	hlen[1] = 0.9
	hlen[2] = 0.6

	x2 = int(cx + clock_cos(a) * (r * hlen[0]))
	y2 = int(cy + clock_sin(a) * (r * hlen[0]))
	hand_sec = layout_create_line("hands", ui, cx, cy, x2, y2, 0.2*mm, cl, flg)

	x2 = int(cx + clock_cos(a) * (r * hlen[1]))
	y2 = int(cy + clock_sin(a) * (r * hlen[1]))
	hand_min = layout_create_line("hands", ui, cx, cy, x2, y2, 0.5*mm, cl, flg)

	x2 = int(cx + clock_cos(a) * (r * hlen[2]))
	y2 = int(cy + clock_sin(a) * (r * hlen[2]))
	hand_hour = layout_create_line("hands", ui, cx, cy, x2, y2, 2*mm, cl, flg)

	clock_cx = cx
	clock_cy = cy
	clock_r = r
}


function ev_action(id, name, argc, x, y)
{
	ui = int(uilayer_alloc("clock", "#DD33DD"))
	mm = mm2pcb_multiplier();
	AddTimer(0, 10, "ev_second", "");
	draw_dial(ui, 50*mm, 50*mm, 30*mm)

	clock_sec = 12
	clock_min = 33
	clock_hour = 9
	set_hands()
}

function set_hand(obj, idx, at     ,ox2,oy2,x2,y2)
{
	ox2 = layout_obj_coord(obj, "OC_P2X")
	oy2 = layout_obj_coord(obj, "OC_P2Y")

	x2 = int(clock_cx + clock_cos(at) * (clock_r * hlen[idx]))
	y2 = int(clock_cy + clock_sin(at) * (clock_r * hlen[idx]))

	layout_obj_move(obj, "OC_P2", x2-ox2, y2-oy2);
}

function set_hands()
{
	set_hand(hand_hour, 2, (clock_hour + clock_min/60 + clock_sec/3600)*5)
	set_hand(hand_min, 1, clock_min + clock_sec/60)
	set_hand(hand_sec, 0, clock_sec)
}


function ev_second(timer_data)
{
	if (hand_sec != "") {
		clock_sec++;
		if (clock_sec >= 60) {
			clock_sec = 0
			clock_min++
			if (clock_min >= 60) {
				clock_min = 0
				clock_hour++
				if (clock_hour >= 12)
					clock_hour = 0;
			}
		}
		set_hands()
	}
}

BEGIN {
	layout_search_empty("hands")
	Bind("ACTE_action", "ev_action");
	action_register("clock", "", "animate an analog clock on a debug layer", "clock()");
}
