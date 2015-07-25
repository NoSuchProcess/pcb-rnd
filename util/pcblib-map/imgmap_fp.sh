#!/bin/sh
pcb=$1.pcb

pcb -x png --dpi 300 --photo-mode $pcb
png=$1.png
html=$1.html

png_dims=`file $png | awk -F "[,]" '{ sub("x", " ", $2); print $2}'`



awk -v "png_dims=$png_dims" -v "png_url=$png" '
BEGIN {
	q = "\""
	sub("^ *", "", png_dims)
	sub(" *$", "", png_dims)
	split(png_dims, A, " +")
	png_sx = A[1]
	png_sy = A[2]
	ne = 0
}

function bump(idx, x, y)
{
	if ((E[idx, "minx"] == "") || (x < E[idx, "minx"]))
		E[idx, "minx"] = x
	if ((E[idx, "maxx"] == "") || (x > E[idx, "maxx"]))
		E[idx, "maxx"] = x
	if ((E[idx, "miny"] == "") || (y < E[idx, "miny"]))
		E[idx, "miny"] = y
	if ((E[idx, "maxy"] == "") || (y > E[idx, "maxy"]))
		E[idx, "maxy"] = y
}

# Element["" "rcy(150, bar-sign)" "C1" "acy150" 22500 95000 -2000 -4500 1 100 ""]
/^Element *[[]/ {
	coords=$0
	sub("Element *[[]", "", coords)
	for(n = 1; n <= 4; n++) {
		match(coords, "[\"][^\"]*[\"]")
		P[n] = substr(coords, RSTART+1, RLENGTH-2)
		sub("[\"][^\"]*[\"]", "", coords)
	}
	split(coords, A, " ")
	E[ne, "cmd"] = P[2]
	E[ne, "cx"] = A[1]
	E[ne, "cy"] = A[2]
	E[ne, "file"] = P[4]
	ne++
	next
}

#ElementLine [-11811 -13006 -11811 -11250 3937]
/^[ \t]*ElementLine *[[]/ {
	sub("ElementLine *[[]", "", $0)
	bump(ne-1, $1, $2)
	bump(ne-1, $3, $4)
	next
}

#ElementArc [-11811 -13006 -11811 -11250 3937]
/^[ \t]*ElementArc *[[]/ {
	sub("ElementArc *[[]", "", $0)
	x = $1
	y = $2
	rx = $3
	ry = $4
	bump(ne-1, x-rx, y-ry)
	bump(ne-1, x+rx, y+ry)
	next
}

#Via[260000 120000 7874 4000 0 3150 "" ""]
/^[ \t]*Via *[[]/ {
	sub("Via *[[]", "", $0)
	pcb_sx = $1
	pcb_sy = $2
	next
}


END {
	scale_x = png_sx/pcb_sx
	scale_y = png_sy/pcb_sy
	print "<html><body>"
	print "<img src=" q png_url q " usemap=\"#fpmap\">"
	print "<map name=\"fpmap\">"
	for(n = 0; n < ne; n++) {
#		print E[n, "minx"], E[n, "maxx"], E[n, "miny"], E[n, "maxy"]
		x1 = int((E[n, "minx"] + E[n, "cx"]) * scale_x)
		x2 = int((E[n, "maxx"] + E[n, "cx"]) * scale_x)
		y1 = int((E[n, "miny"] + E[n, "cy"]) * scale_y)
		y2 = int((E[n, "maxy"] + E[n, "cy"]) * scale_y)
		if (x1 < 0)
			x1 = 0
		if (y1 < 0)
			y1 = 0
		if (x1 > x2) {
			tmp = x1
			x1 = x2
			x2 = tmp
		}
		if (y1 > y2) {
			tmp = y1
			y1 = y2
			y2 = tmp
		}
		x1 -= 5
		y1 -= 5
		x2 += 5
		y2 += 5
#		print n, x1, y1, x2, y2, E[n, "cmd"]
		cmd =  E[n, "cmd"]
		if (cmd ~ "^[^ ]*[(]")
			url="http://igor2.repo.hu/cgi-bin/pcblib-param.cgi?cmd=" cmd
		else
			url="http://igor2.repo.hu/cgi-bin/pcblib-static.cgi?fp=" E[n, "file"]
		gsub(" ", "+", url)
		print "<area shape=\"rect\" coords=" q x1 "," y1 "," x2 "," y2 q " href=" q url q " alt=" q E[n, "cmd"] q ">"
	}
	print "</map>"
	print "</body></html>"
}

' < $pcb > $html


