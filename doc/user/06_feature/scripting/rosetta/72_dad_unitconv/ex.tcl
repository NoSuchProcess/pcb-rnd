proc uconv {} {
	global inp res1 res2 res3

	if {[dad "uconv" "exists"] == 0} {
		dad "uconv" "new"
		dad "uconv" "begin_hbox"
			set inp [dad "uconv" "coord" 0 "100m"]
				dad "uconv" "onchange" "uconv_update"
			dad "uconv" "begin_vbox"
				set res1 [dad "uconv" "label" "-"]
				set res2 [dad "uconv" "label" "-"]
				set res3 [dad "uconv" "label" "-"]
			dad "uconv" "end"
		dad "uconv" "end"
		dad "uconv" "run" "Unit converter" ""
	}
}

proc uconv_update {} {
	global inp res1 res2 res3

	dad "uconv" "set" $res1 "[dad uconv get $inp mm] mm"
	dad "uconv" "set" $res2 "[dad uconv get $inp mil] mil"
	dad "uconv" "set" $res3 "[dad uconv get $inp inch] inch"
}

fgw_func_reg "uconv"
fgw_func_reg "uconv_update"
