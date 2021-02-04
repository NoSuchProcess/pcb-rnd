#!/bin/sh

ROOT="../.."
SRC="$ROOT/src"
PCBRND="./pcb-rnd"
GLOBARGS="-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1"


run_pcb_rnd() {
local brd="`pwd`/$1"

shift 1

(
echo '
message(*** PRE  ***)
Report(DrillReport)
'

while test $# -gt 0
do
	echo "LoadVendorFrom(\"`pwd`/$1\", pure)"
	shift 1
done

echo '
ApplyVendor()

message(*** POST ***)
Report(DrillReport)
') | (
	cd $SRC
	export LD_LIBRARY_PATH=../src_3rd/librnd-local/src
	./pcb-rnd $GLOBARGS --gui batch $brd
) | awk '
# remove full path filenames
/Loaded.*from/ {
	line=$0
	sub("[/].*[/]", "", line)
	print line
	next
}
{ print $0 }
'

}

run_test()
{
	local tname="$1"
	shift 1

	run_pcb_rnd "$@" > $tname.out

	diff -u $tname.ref $tname.out
	if test "$?" = "0"
	then
		rm $tname.out
		return 0
	fi
	return 1
}

run_test skip_attr    testbrd.lht skip_attr.lht
run_test skip_descr   testbrd.lht skip_descr.lht
run_test skip_refdes  testbrd.lht skip_refdes.lht
run_test skip_value   testbrd.lht skip_value.lht

run_test pure         testbrd.lht pure1.lht pure2.lht

run_test round        testbrd.lht round.lht
run_test round_up     testbrd.lht round_up.lht
run_test round_down   testbrd.lht round_down.lht
run_test round_near   testbrd.lht round_near.lht

