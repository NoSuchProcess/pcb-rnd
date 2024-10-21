#!/bin/sh
ROOT=../..
#DBG=cdb

PWD=`pwd`

SRC=$ROOT/src
if test -x $SRC/pcb-rnd.wrap
then
	PCB_RND=./pcb-rnd.wrap
else
	PCB_RND=./pcb-rnd
fi


run_pcb_rnd()
{
	(cd $ROOT/src && $DBG $PCB_RND "$@") 2>&1 | awk '
		/^[*][*][*] Exporting:/ { next }
		/Warning: footprint library list error on/ { next }
		/^[ \t]*$/ { next }
		{ print $0 }
	'
}

do_test()
{
	local refs r o
	run_pcb_rnd -x cam holes --outfile $PWD/  $PWD/board.lht

	(
		for r in *.exc.ref
		do
			o=${r%.ref}
			diff -u $r $o
		done
	)
}

do_clean()
{
	rm -f *.exc
}

cmd="$1"
shift 1

case "$cmd" in
	test) do_test ;;
	clean) do_clean ;;
esac

