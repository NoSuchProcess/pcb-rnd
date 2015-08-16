PkgLoad pcb-rnd-gpmi/hid 0
PkgLoad pcb-rnd-gpmi/layout 0
PkgLoad pcb-rnd-gpmi/dialogs 0

set hid [hid_create "drill" "drill list export"]
set attr_path   [hid_add_attribute $hid "filename" "name of the output file" "HIDA_Path" 0 0 "drill.txt"]
set attr_format [hid_add_attribute $hid "format"   "file format" "HIDA_Enum" 0 0 "CSV|TSV|text"]
hid_register $hid

set fmt 0
set conv [mm2pcb_multiplier]
set channel -1
set green_light 0

proc make_gc {event_id hid gc} {
	global channel attr_path attr_format fmt

	if {$channel == -1} {
		if {[catch {open [hid_get_attribute $hid $attr_path] "w"} channel]} {
			dialog_report "Error exporting drill" "Could not open file [hid_get_attribute $hid $attr_path] for\nwriting, exporting drill failed."
			return
		}
		set fmt [hid_get_attribute $hid $attr_format]
	}
}

proc destroy_gc {event_id hid gc} {
	global channel

	if {$channel > -1} {
		close $channel
		set channel -1
	}
}

proc set_layer {event_id hid name group empty} {
	global green_light

	if { $name eq "topassembly" } { set green_light 1 } { set green_light 0 }
}

proc fill_circle {event_id hid gc cx cy r} {
	global channel conv fmt green_light

	if {$green_light} {
		set cx  [expr $cx / $conv]
		set cy  [expr $cy / $conv]
		set dia [expr $r  / $conv * 2]

		if {$fmt eq "CSV"} {
			puts $channel "$cx,$cy,$dia"
		} elseif {$fmt eq "TSV"} {
			puts $channel "$cx	$cy	$dia"
		} elseif {$fmt eq "text"} {
			puts $channel "$cx    $cy    $dia"
		}
	}
}

Bind HIDE_make_gc make_gc
Bind HIDE_destroy_gc destroy_gc
Bind HIDE_set_layer set_layer
Bind HIDE_fill_circle fill_circle
