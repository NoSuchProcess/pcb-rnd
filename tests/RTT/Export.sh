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
		png)
			fmt_args="--dpi 1200"
			ext=.png
			;;
		svg)
			ext=.svg
			;;
	esac
}

cmp_fmt()
{
	local ref="$1" out="$2"
	case "$fmt" in
		png)
			echo "$ref" "$out"
	esac
}

while test $# -gt 0
do
	case "$1"
	in
		-f|-x) fmt=$2; shift 1;;
		-b) pcb_rnd_bin=$2; shift 1;;
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

$pcb_rnd_bin -x "$fmt" $fmt_args "$fn"

base=${fn%%.pcb}
ref_fn=ref/$base$ext
fmt_fn=$base$ext
out_fn=out/$base$ext
if test -f "$fmt_fn"
then
	mv $fmt_fn out
fi
cmp_fmt "$ref_fn" "$out_fn"
