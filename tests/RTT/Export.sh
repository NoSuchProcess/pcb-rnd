#!/bin/sh

TRUNK=../..
if test -z "$pcb_rnd_bin"
then
	pcb_rnd_bin="$TRUNK/src/pcb-rnd"
fi

fmt_args=""

set_fmt_args()
{
	case "$fmt" in
		bom) ext=.bom ;;
		dsn) ext=.dsn ;;
		IPC-D-356) ext=.net;;
		ps) ext=.ps ;;
		XY) ext=.xy ;;
		png)
			fmt_args="--dpi 1200"
			ext=.png
			;;
		gerber)
# multifile: do not set ext
			;;
		remote)
			ext=.remote
			;;
		svg)
			ext=.svg
			;;
	esac
}

move_out()
{
	local raw_out="$1" final_out="$2"
	case "$fmt" in
		gerber)
			mkdir -p $final_out.gbr
			mv $raw_out.*.gbr $final_out.gbr
			;;
		remote|ps)
			gzip $raw_out
			mv $raw_out.gz $final_out.gz
			;;
		*)
			# common, single file output
			if test -f "$fmt_fn"
			then
				mv $raw_out $final_out
			fi
			;;
	esac
}

cmp_fmt()
{
	local ref="$1" out="$2"
	case "$fmt" in
		png)
			echo "$ref" "$out"
			;;
		*)
			# simple text files: byte-to-byte match required
			diff -u "$ref" "$out"
			;;
	esac
}

run_test()
{
	local fn="$1"

	$pcb_rnd_bin -x "$fmt" $fmt_args "$fn"

	base=${fn%%.pcb}
	ref_fn=ref/$base$ext
	fmt_fn=$base$ext
	out_fn=out/$base$ext

	move_out "$fmt_fn" "$out_fn"
	cmp_fmt "$ref_fn" "$out_fn"
}

while test $# -gt 0
do
	case "$1"
	in
		-f|-x) fmt=$2; shift 1;;
		-b) pcb_rnd_bin=$2; shift 1;;
		-a) all=1; shift 1;;
		*)
			if test -z "$fn"
			then
				fn="$1"
			else
				echo "unknown switch $1; try --help" >&2
				exit 1
			fi
			;;
	esac
	shift 1
done

if test -z "$fmt"
then
	echo "need a format" >&2
fi

set_fmt_args

if test "$all" -gt 0
then
	for n in `ls *.lht *.pcb`
	do
		run_test "$n"
	done
else
	run_test "$fn"
fi
