#!/bin/sh

TRUNK=../..

all=0
valg=0
libdir=`pwd`
global_args="-c rc/library_search_paths=lib -c rc/export_basename=1 -c design/fab_author=TEST -c rc/quiet=1 -c rc/default_pcb_file={} -c rc/default_font_file=$libdir/default_font"
test_announce=0
verbose=0
CONVERT=convert
COMPARE=compare
SCCBOX=$TRUNK/scconfig/sccbox

polyb=`grep "[ \t]*#define[ \t]*PCB_WANT_POLYBOOL[ \t]*1" $TRUNK/config.h`
if test ! -z "$polyb"
then
	REF_FLAVOR=polybool
else
	polyb2=`grep "[ \t]*#define[ \t]*PCB_WANT_POLYBOOL2[ \t]*1" $TRUNK/config.h`
	if test ! -z "$polyb2"
	then
		REF_FLAVOR=polybool2
	fi
fi

if test -z "$pcb_rnd_bin"
then
# running from source
	pcb_rnd_cd="$TRUNK/src"
	pcb_rnd_bin="./pcb-rnd"

	if test -x $TRUNK/src/pcb-rnd.wrap
	then
		pcb_rnd_bin="./pcb-rnd.wrap"
	else
		pcb_rnd_bin="./pcb-rnd"
	fi
fi

fmt_args=""

scc_mktemp()
{
	$SCCBOX mktemp -p "$libdir" "$@"
}

# portable sed -i implementation with temp files
sedi()
{
	local sc n
	sc=$1
	shift 1
	for n in "$@"
	do
		sed "$sc" < "$n" > "$n.sedi" && mv "$n.sedi" "$n"
	done
}

# Execute pcb-rnd, optionally from trunk/src/, optionally wrapped in valgrind
run_pcb_rnd()
{
	(
		if test ! -z "$pcb_rnd_cd"
		then
			cd "$pcb_rnd_cd"
		fi
		$valgr $pcb_rnd_bin "$@"
	)
}

need_convert()
{
	if test -z "`$CONVERT 2>/dev/null | grep ImageMagick`" -o -z "`$COMPARE 2>/dev/null | grep ImageMagick`"
	then
		echo "WARNING: ImageMagick convert(1) or compare(1) not found - bitmap compare will be skipped."
		CONVERT=""
	fi
}

set_fmt_args()
{
	case "$fmt" in
		bboard) need_convert; ext=.bbrd.png ;;
		nelma) ext=.nelma.em ;;
		bom) ext=.bom ;;
		dsn) ext=.dsn ;;
		gcode) ext=.gcode;;
		IPC-D-356) ext=.net;;
		ps)
			ext=.ps
			fmt_args="-c plugins/draw_fab/omit_date=1"
			;;
		eps)
			ext=.eps
#			fmt_args="-c plugins/draw_fab/omit_date=1"
			;;
		XY) ext=.xy ;;
		openscad)
			ext=.scad
			fmt_args="--silk"
			;;
		png)
			need_convert
			fmt_args="--dpi 1200"
			ext=.png
			;;
		gerber)
			fmt_args="--cross-sect -c plugins/draw_fab/omit_date=1"
# multifile: do not set ext
			;;
		excellon)
			ext=""
			;;
		remote)
			ext=.remote
			;;
		svg)
			ext=.svg
			;;
		fidocadj)
			ext=.fcd
			;;
	esac
}

move_out()
{
	local raw_out="$1" final_out="$2" n

# remove variable sections
	case "$fmt" in
		dsn) sedi 's/[(]host_version[^)]*[)]/(host_version "<version>")/g' $raw_out ;;
		bom|XY) sedi "s/^# Date:.*$/# Date: <date>/" $raw_out ;;
		IPC-D-356)
			sedi '
				s/^C  File created on .*$/C  File created on <date>/
				s/^C  IPC-D-356 Netlist generated by.*$/C  IPC-D-356 Netlist generated by <version>/
			' $raw_out ;;
		nelma)
			sedi 's@/[*] Made with PCB Nelma export HID [*]/.*@/* banner */@g' $raw_out ;;
		gerber)
			sedi '
				s/^G04 CreationDate:.*$/G04 CreationDate: <date>/
				s/^G04 Creator:.*$/G04 Creator: <version>/
			' $raw_out.*.gbr
# do not save or compare the csect yet
			rm $raw_out.*csect*.gbr
			;;
		ps)
			sedi '
				s@%%CreationDate:.*@%%CreationDate: date@
				s@%%Creator:.*@%%Creator: pcb-rnd@
				s@%%Version:.*@%%Version: ver@
				s@^[(]Created on.*@(Created on date@
				/^matrix defaultmatrix setmatrix/ d;
			' $raw_out
			;;
		gcode)
			for n in `ls $raw_out.*.cnc 2>/dev/null`
			do
			awk '
				/^[(] / && (NR < 5) { next } # skip date
				{ print $0 }
			' $n > $n.tmp
			mv $n.tmp $n
			done
	esac

# move the output file(s) to their final subdir (for multifile formats) and/or
# compress them (for large text files)
	case "$fmt" in
		bboard)
			mv ${raw_out%%.bbrd.png}.png $final_out
			;;
		gerber)
			mkdir -p $final_out.gbr
			mv $raw_out.*.gbr $final_out.gbr
			;;
		excellon)
			mkdir -p $final_out.exc
			mv $raw_out.*.cnc $final_out.exc 2>/dev/null
			;;
		nelma)
			mv $raw_out $final_out
			# do not test the pngs for now
			rm -f ${raw_out%%.em}*.png
			;;
		remote|ps)
			gzip $raw_out
			mv $raw_out.gz $final_out.gz
			;;
		gcode)
			mkdir -p $final_out
			files=`ls $raw_out.*.cnc 2>/dev/null`
			if test ! -z "$files"
			then
				mv $files $final_out
				rm -f ${raw_out%%.gcode}.*.png
			fi
			;;
		*)
			# common, single file output
			if test -f "$raw_out"
			then
				mv $raw_out $final_out
			fi
			;;
	esac
}

cmp_fmt()
{
	local ref="$1" out="$2" n bn otmp
	case "$fmt" in
		png)
			if test ! -z "$CONVERT"
			then
				bn=`basename $out`
				res=`$COMPARE "$ref" "$out"  -metric AE  diff/$bn 2>&1`
				case "$res" in
					*widths*)
						otmp=$out.599.png
						$CONVERT -crop 599x599+0x0 $out  $otmp
						res=`$COMPARE "$ref" "$otmp" -metric AE  diff/$bn 2>&1`
						;;
				esac
				test "$res" -lt 8 && rm diff/$bn
				test "$res" -lt 8
			fi
			;;
		bboard)
			if test ! -z "$CONVERT"
			then
				bn=`basename $out`
				res=`$COMPARE "$ref" "$out"  -metric AE  diff/$bn 2>&1`
				test "$res" -lt 8 && rm diff/$bn
				test "$res" -lt 8
			fi
			;;
		gerber)
			for n in `ls $ref.gbr/*.gbr 2>/dev/null`
			do
				bn=`basename $n`
				alt_n=ref-$REF_FLAVOR/${n#ref/}
				if test -f $alt_n
				then
					# compare to flavored ref
					diff -u "$alt_n" "$out.gbr/$bn"
				else
					# compare to the generic ref
					diff -u "$n" "$out.gbr/$bn"
				fi
			done
			;;
		excellon)
			for n in `ls $ref.exc/*.cnc 2>/dev/null`
			do
				bn=`basename $n`
				diff -u "$n" "$out.exc/$bn"
			done
			;;
		ps)
			zcat "$ref.gz" > "$ref"
			zcat "$out.gz" > "$out"
			diff -u "$ref" "$out" && rm "$ref" "$out"
			;;
		*)
			# simple text files: byte-to-byte match required
			diff -u "$ref" "$out"
			;;
	esac
}

# Remove known, expected error messages
stderr_filter()
{
	local pats="Couldn't find default.pcb|No preferred unit format info available for|has no font information, using default font|Log produced after failed export|Exporting empty board|[*][*][*] Exporting:|^.pcb-rnd:stderr.[ \t]*$|Warning: footprint library list"

	# format specific extras
	case "$fmt" in
		gerber) pats="Can't export polygon as G85 slot|please use lines for slotting|$pats" ;;
		excellon) pats="Excellon: can not export [a-z]* (some features may be missing from the export)|$pats" ;;
		svg) pats="Can't draw elliptical arc on svg|$pats";;
	esac

	# have to use awk for this since grep "|" is not portable
	awk -v pats="$pats" '

		# load patterns
		BEGIN {
			v = split(pats, A, "[|]")
		}

		# filter
		{
			for(n = 1; n <= v; n++)
				if ($0 ~ A[n])
					next
		}
	'
}

run_test()
{
	local ffn fn="$1" valgr res res2

	if test "$valg" -gt 0
	then
		export valgr="valgrind -v --leak-check=full --show-leak-kinds=all --log-file=`pwd`/$fn.vlog"
	fi

	if test "$verbose" -gt 0
	then
		echo "=== Test: $fmt $fn ==="
	fi

	# run and save stderr in file res2 and stdout in variable res
	res2=`scc_mktemp`
	ffn="`pwd`/$fn"
	res=`run_pcb_rnd -x "$fmt" $global_args $fmt_args "$ffn" 2>$res2`

	# special case error: empty design is not exported; skip the file
	if test ! -z "$res"
	then
		case "$res" in
			*"export empty board"*) rm $res2; return 0 ;;
		esac
	fi

	# print error messages to the log
	sed "s/^/[pcb-rnd:stderr]  /" < $res2 | stderr_filter >&2
	rm $res2

	base=${fn%%.pcb}
	base=${base%%.lht}
	fmt_fn=$base$ext
	out_fn=out/$base$ext

	# the ref file is from the flavor we figured from config.h, e.g. for polybool
	# or as a fallback the standard set if there is no flavor-specific file for
	# this input
	ref_fn=ref-$REF_FLAVOR/$base$ext
	if test ! -f $ref_fn -a ! -f $ref_fn.gz
	then
		ref_fn=ref/$base$ext
	fi


	move_out "$fmt_fn" "$out_fn"
	cmp_fmt "$ref_fn" "$out_fn"
}

while test $# -gt 0
do
	case "$1"
	in
		--list)
			run_pcb_rnd -x -list-
			exit 0;;
		-t) test_announce=1;;
		-f|-x) fmt=$2; shift 1;;
		-b) pcb_rnd_bin=$2; shift 1;;
		-a) all=1;;
		-V) valg=1;;
		-v) verbose=1;;
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

if test "$test_announce" -gt 0
then
	echo -n "$fmt: ... "
fi

bad=""
if test "$all" -gt 0
then
	for n in `ls *.lht *.pcb 2>/dev/null`
	do
		case $n in
			*Proto.lht) continue;;
			*Proto.pcb) continue;;
			*default.lht) continue;;
			*default.pcb) continue;;
			*) run_test "$n" || bad="$bad $n" ;;
		esac
	done
else
	run_test "$fn" || bad="$bad $n"
fi

if test ! -z "$bad"
then
	if test "$verbose" -gt 0
	then
		echo "$fmt: ... BROKEN: $bad"
	else
		echo "$fmt: ... BROKEN"
	fi
	exit 1
else
	echo "ok"
	exit 0
fi


