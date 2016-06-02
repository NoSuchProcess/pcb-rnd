#!/bin/sh
src_dir=../../src
pwd=`pwd`

# $1 must be a .pcb file name
run_pcb()
{
	local fn
	fn="$pwd/$1"
	shift 1
	(
		cd "$src_dir"
		./pcb-rnd --gui batch "$fn" "$@"
	)
}


set_flag_and_save()
{
	local name="`basename $1`" newf expect="Flags(\"$3\")" res
	if test -z "$name"
	then
		name="0"
	fi
	rm $name 2>/dev/null
	echo "
conf(set, $1, $2, design)
SaveTo(LayoutAs, $pwd/$name.pcb)
dumpconf(lihata,design)
" | run_pcb vanilla.pcb >$name.log 2>&1
	newf=`grep ^Flags $name.pcb`
	test "$newf" = "$expect"
	res=$?
	if test $res != 0
	then
		echo "$1 broken: expected '$expect' got '$newf'"
	fi
	return $res
}


set_flag_and_save "plugins/mincut/enable" "true" "enablemincut"
set_flag_and_save "editor/show_number" "true" "shownumber"
set_flag_and_save "editor/show_drc" "true" "showdrc"
set_flag_and_save "editor/rubber_band_mode" "true" "rubberband"
set_flag_and_save "editor/auto_drc" "true" "autodrc"
set_flag_and_save "editor/all_direction_lines" "true" "alldirection"
set_flag_and_save "editor/swap_start_direction" "true" "swapstartdir"
set_flag_and_save "editor/unique_names" "true" "uniquename"
set_flag_and_save "editor/clear_line" "true" "clearnew"
set_flag_and_save "editor/full_poly" "true" "newfullpoly"
set_flag_and_save "editor/snap_pin" "true" "snappin"
set_flag_and_save "editor/orthogonal_moves" "true" "orthomove"
set_flag_and_save "editor/live_routing" "true" "liveroute"
set_flag_and_save "editor/enable_stroke" "true" "enablestroke"

