#!/bin/sh

pcb=pcb-rnd
CONVERT=convert
COMPARE=compare
TRUNK=../..
srcdir=$TRUNK/src

global_args="-c rc/library_search_paths=lib -c plugins/draw_fab/omit_date=1 -c design/fab_author=TEST -c rc/quiet=1 -c rc/default_font_file=$srcdir/default_font"

test_svg='
	base
	ph --photo-mode
	phn --photo-mode --photo-noise
	opa --opacity 50
	flip --flip
	as --as-shown
	ts --true-size
'

test_png='
	base --dpi 200
	xmax --dpi 200 --x-max 50
	ymax --dpi 200 --y-max 50
	xymax --dpi 200 --xy-max 50
	as --dpi 200 --as-shown
	mono --dpi 200 --monochrome
	alpha --dpi 200 --use-alpha
	bloat --dpi 200 --png-bloat 0.5mm
	ph --dpi 200 --photo-mode --photo-plating copper
	phx --dpi 200 --photo-mode --photo-flip-x --photo-plating copper
	phy --dpi 200 --photo-mode --photo-flip-y --photo-plating copper
	phmr --dpi 200 --photo-mode --photo-mask-colour red  --photo-plating copper
	phpg --dpi 200 --photo-mode --photo-plating gold
	phsb --dpi 200 --photo-mode --photo-silk-colour black --photo-plating copper
'
# need to Use --photo-plating copper to avoid random noise

test_eps='
	base
	sc --eps-scale 0.6
	as --as-shown
	mono --monochrome
'

test_ps='
	base
	dh --drill-helper --drill-helper-size 0.1mm
	am --no-align-marks
	outl --no-outline
	mirr --mirror
	amirr --mirror --no-auto-mirror
	fill --fill-page
	clr --ps-color
	inv --ps-invert
	scale --scale 4
	dcp --no-drill-copper
	leg --no-show-legend --no-show-toc
	1 --single-page --no-show-toc
'

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


# $1: test file
# $2: output ext
# $3: test args, one per line
# $4: optional sedi script to make the output portable
gen_any()
{
	local outfn

	echo "$3" | while read name args
	do
		if test ! -z "$name"
		then
			outfn="out/$1.$name.$2"
			$pcb $global_args $lead "$outfn" "$1.lht" $args
			if test ! -z "$4"
			then
				sedi "$4" "$outfn"
			fi
		fi
	done
}


need_convert()
{
	if test -z "`$CONVERT 2>/dev/null | grep ImageMagick`" -o -z "`$COMPARE 2>/dev/null | grep ImageMagick`"
	then
		echo "WARNING: ImageMagick convert(1) or compare(1) not found - bitmap compare will be skipped."
		CONVERT=""
	fi
}

cmp_fmt()
{
	local fmt="$1" ref="$2" out="$3" n bn otmp
	case "$fmt" in
		png)
			if test ! -z "$CONVERT"
			then
				bn=`basename $out`
				res=`$COMPARE "$ref" "$out"  -metric AE  diff/$bn 2>&1`
#				case "$res" in
#					*widths*)
#						otmp=$out.599.png
#						$CONVERT -crop 599x599+0x0 $out  $otmp
#						res=`$COMPARE "$ref" "$otmp" -metric AE  diff/$bn 2>&1`
#						;;
#				esac
				test "$res" -lt 8 && rm diff/$bn
				test "$res" -lt 8
			fi
			;;
		svg|eps|ps)
			zcat "$ref.gz" > "$ref"
			diff -u "$ref" "$out" && rm "$ref"
			;;
		*)
			# simple text files: byte-to-byte match required
			diff -u "$ref" "$out"
			;;
	esac
}

# $1: test file
# $2: output ext and format
# $3: test args, one per line
cmp_any()
{
	local rv=0

	echo "$3" | while read name args 
	do
		if test ! -z "$name"
		then
			echo -n "$1.$name.$2... "
			cmp_fmt $2 ref/$1.$name.$2 out/$1.$name.$2
			if test "$?" -eq 0
			then
				echo "ok"
			else
				echo "BROKEN"
				rv=1
			fi
		fi
	done
	return $rv
}




gen_svg()
{
	lead="-x svg --outfile"
	gen_any layers svg "$test_svg"
}

cmp_svg()
{
	cmp_any layers svg "$test_svg"
}

gen_png()
{
	lead="-x png --outfile"
	gen_any layers png "$test_png"
}

cmp_png()
{
	cmp_any layers png "$test_png"
}

gen_eps()
{
	lead="-x eps --eps-file"
	gen_any layers eps "$test_eps"
}

cmp_eps()
{
	cmp_any layers eps "$test_eps"
}

gen_ps()
{
	lead="-x ps --psfile"
	gen_any layers ps "$test_ps" '
		s@%%CreationDate:.*@%%CreationDate: date@
		s@%%Creator:.*@%%Creator: pcb-rnd@
		s@%%Version:.*@%%Version: ver@
		s@^[(]Created on.*@(Created on date@
	'
}

cmp_ps()
{
	cmp_any layers ps "$test_ps"
}

mkdir -p out diff

case "$1" in
	svg)
		gen_svg
		cmp_svg
		;;
	png)
		need_convert
		gen_png
		cmp_png
		;;
	eps)
		gen_eps
		cmp_eps
		;;
	ps)
		gen_ps
		cmp_ps
		;;
	"")
		echo "generating svg..."
		gen_svg
		cmp_svg

		echo "generating png..."
		need_convert
		gen_png
		cmp_png

		echo "generating eps..."
		gen_eps
		cmp_eps

		echo "generating ps..."
		gen_ps
		cmp_ps
		;;
	*)
		echo "Invalid format to test" >&2
		;;
esac
