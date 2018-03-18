#!/bin/sh
src_dir=../../src
pwd=`pwd`

# $1 must be a .pcb file name
run_pcb()
{
	local fn
	if test ! -z "$1"
	then
		fn="$pwd/$1"
		shift 1
	fi
	(
		cd "$src_dir"
		if test -z "$fn"
		then
			./pcb-rnd --gui batch "$@"
		else
			./pcb-rnd --gui batch "$fn" "$@"
		fi
	)
}


test_flag()
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
		echo "$1 save broken: expected '$expect' got '$newf'"
		return 1
	fi


	echo "###### load test" >> $name.log
	echo "
conf(reset, system)
conf(reset, project)
LoadFrom(Layout, $pwd/$name.pcb)
SaveTo(LayoutAs, $pwd/$name.2.pcb)
dumpconf(lihata,design)
" | run_pcb >>$name.log 2>&1

	newf=`grep ^Flags $name.2.pcb`
	test "$newf" = "$expect"
	res=$?
	if test $res != 0
	then
		echo "$1 load broken: expected '$expect' got '$newf'"
		return 1
	fi

	return 0
}

diag=`cd "$src_dir" && ./pcb-rnd --dump-plugins | grep diag`
if test -z "$diag"
then
	echo "pcbflags: can't run tests because the diag plugin is not compiled."
	exit 0
fi


test_flag "plugins/mincut/enable" "true" "enablemincut"
test_flag "editor/show_number" "true" "shownumber"
test_flag "editor/show_drc" "true" "showdrc"
test_flag "editor/rubber_band_mode" "true" "rubberband"
test_flag "editor/auto_drc" "true" "autodrc"
test_flag "editor/all_direction_lines" "true" "alldirection"
test_flag "editor/swap_start_direction" "true" "swapstartdir"
test_flag "editor/unique_names" "true" "uniquename"
test_flag "editor/clear_line" "true" "clearnew"
test_flag "editor/full_poly" "true" "newfullpoly"
test_flag "editor/snap_pin" "true" "snappin"
test_flag "editor/orthogonal_moves" "true" "orthomove"
test_flag "editor/live_routing" "true" "liveroute"
test_flag "editor/only_names" "true" "onlynames"
test_flag "editor/lock_names" "true" "locknames"
test_flag "editor/hide_names" "true" "hidenames"
test_flag "editor/thin_draw" "true" "thindraw"
test_flag "editor/thin_draw_poly" "true" "thindrawpoly"
test_flag "editor/local_ref" "true" "localref"
test_flag "editor/check_planes" "true" "checkplanes"
test_flag "editor/description" "true" "description"
test_flag "editor/name_on_pcb" "true" "nameonpcb"
