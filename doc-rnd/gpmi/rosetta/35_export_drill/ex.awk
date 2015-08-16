BEGIN {
	PkgLoad("pcb-rnd-gpmi/hid", 0)
	PkgLoad("pcb-rnd-gpmi/layout", 0)
	PkgLoad("pcb-rnd-gpmi/dialogs", 0)

	hid = hid_create("drill", "drill list export")
	attr_path =  hid_add_attribute(hid, "filename", "name of the output file", "HIDA_Path", 0, 0, "drill.txt")
	attr_format = hid_add_attribute(hid, "format",  "file format", "HIDA_Enum", 0, 0, "CSV|TSV|text")
	hid_register(hid)

	fmt = 0
	conv = mm2pcb_multiplier()
	channel = ""
	green_light = 0
}

function make_gc(event_id, hid, gc) {
	if (channel == "") {
		channel = hid_get_attribute(hid, attr_path)
# awk: can't check whether the file can be open here
		fmt = hid_get_attribute(hid, attr_format)
	}
}

function destroy_gc(event_id, hid, gc) {
	if (channel != "") {
		close(channel)
		channel = ""
	}
}

function set_layer(event_id, hid, name, group, empty) {
	if (name == "topassembly")
		green_light = 1
	else
		green_light = 0
}

function fill_circle(event_id, hid, gc, cx, cy, r      ,dia) {
	if (green_light) {
		cx  = cx / conv
		cy  = cy / conv
		dia = r  / conv * 2

		if (fmt == "CSV") {
			print cx "," cy "," dia > channel
		} else if (fmt == "TSV") {
			print cx "\t" cy "\t" dia > channel
		} else if (fmt == "text") {
			print cx "    " cy "    " dia > channel
		}
	}
}

BEGIN {
	Bind("HIDE_make_gc", "make_gc")
	Bind("HIDE_destroy_gc", "destroy_gc")
	Bind("HIDE_set_layer", "set_layer")
	Bind("HIDE_fill_circle", "fill_circle")
}
