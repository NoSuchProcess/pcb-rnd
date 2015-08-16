PkgLoad("pcb-rnd-gpmi/hid", 0)
PkgLoad("pcb-rnd-gpmi/layout", 0)
PkgLoad("pcb-rnd-gpmi/dialogs", 0)

hid = hid_create("drill", "drill list export")
attr_path =  hid_add_attribute(hid, "filename", "name of the output file", "HIDA_Path", 0, 0, "drill.txt")
attr_format = hid_add_attribute(hid, "format",  "file format", "HIDA_Enum", 0, 0, "CSV|TSV|text")
hid_register(hid)

fmt = 0
conv = mm2pcb_multiplier()
channel = nil
green_light = 0

function make_gc(event_id, hid, gc)
	if (channel == nil)
	then
		channel = io.open(hid_get_attribute(hid, attr_path), "w")
		if (channel == nil)
		then
			dialog_report("Error exporting drill", "Could not open file [hid_get_attribute $hid $attr_path] for\nwriting, exporting drill failed.")
			return
		end
		fmt = hid_get_attribute(hid, attr_format)
	end
end

function destroy_gc(event_id, hid, gc)
	if channel ~= nil
	then
		channel:close()
		channel = nil
	end
end

function set_layer(event_id, hid, name, group, empty)
	if name == "topassembly"
	then
		green_light = 1
	else
		green_light = 0
	end
end

function fill_circle(event_id, hid, gc, cx, cy, r)
	if green_light
	then
		cx  = cx / conv
		cy  = cy / conv
		local dia = r  / conv * 2

		if fmt == "CSV" then
			channel:write(cx .. "," .. cy .. "," .. dia .. "\n")
		elseif (fmt == "TSV") then
			channel:write(cx .. "\t" .. cy .. "\t" .. dia .. "\n")
		elseif (fmt == "text") then
			channel:write(cx .. "    " .. cy .. "    " .. dia .. "\n")
		end
	end
end

Bind("HIDE_make_gc", "make_gc")
Bind("HIDE_destroy_gc", "destroy_gc")
Bind("HIDE_set_layer", "set_layer")
Bind("HIDE_fill_circle", "fill_circle")
