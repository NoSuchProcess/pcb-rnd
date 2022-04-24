#!/bin/sh

pcb=pcb-rnd

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
	ph --dpi 200 --photo-mode
	phx --dpi 200 --photo-mode --photo-flip-x
	phy --dpi 200 --photo-mode --photo-flip-y
	phmr --dpi 200 --photo-mode --photo-mask-colour red
	phpg --dpi 200 --photo-mode --photo-plating gold
	phsb --dpi 200 --photo-mode --photo-silk-colour black
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

gen_svg()
{
	lead="-x svg --outfile"
	gen_any layers svg "$test_svg"
}

gen_png()
{
	lead="-x png --outfile"
	gen_any layers png "$test_png"
}


mkdir -p out

gen_svg
gen_png

