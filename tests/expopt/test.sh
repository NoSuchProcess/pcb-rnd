#!/bin/sh

pcb=pcb-rnd
CONVERT=convert
COMPARE=compare

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


# $1: test file
# $2: output ext
# $3: test args, one per line
gen_any()
{
	echo "$3" | while read name args 
	do
		if test ! -z "$name"
		then
			$pcb $lead out/$1.$name.$2 $1.lht $args
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
		ps)
			zcat "$ref.gz" > "$ref"
			zcat "$out.gz" > "$out"
			diff -u "$ref" "$out" && rm "$ref" "$out"
			;;
		svg|eps)
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
	"")
		echo "generating svg..."
		gen_svg
		cmp_svg

		echo "generating png..."
		need_convert
		gen_png
		cmp_png
		;;
	*)
		echo "Invalid format to test" >&2
		;;
esac
