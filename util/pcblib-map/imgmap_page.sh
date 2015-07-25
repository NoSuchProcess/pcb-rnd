#!/bin/sh
pcb=$1.pcb

pcb -x png --dpi 100 --photo-mode $pcb
png=$1.png

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
	te = 0
}


#	Polygon("clearpoly")
#	(
#		[5000 2500] [277500 2500] [277500 547500] [5000 547500] 
#	)

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

/^[ \t]*Polygon[(]/ {
	getline
	getline
	gsub("[[\\]]", "", $0)
	for(n = 1; n < NF; n+=2)
		bump(ne, $n, $(n+1))
	ne++
	next
}

#	Text[296338 119704 0 670 "PARAMETRIC" "clearline"]
/^[ \t]*Text[[]/ {
	sub("^[ \t]*Text[[]", "", $0)
	T[te, "x"] = $1
	T[te, "y"] = $2
	T[te, "text"] = $5
	gsub("[\"]", "", T[te, "text"])
	te++
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
		x1=(E[n, "minx"] + E[n, "cx"])
		x2=(E[n, "maxx"] + E[n, "cx"])
		y1=(E[n, "miny"] + E[n, "cy"])
		y2=(E[n, "maxy"] + E[n, "cy"])
		t = ""


		for(i = 0; i < te; i++) {
			x=T[i, "x"]
			y=T[i, "y"]
			if ((x>=x1) && (x<=x2) && (y>=y1) && (y<=y2)) {
				t = i
				break;
			}
		}
#		print n, x1, y1, x2, y2, "|", t, T[t, "x"], T[t, "y"], T[t, "text"] > "/dev/stderr"

		x1 = int(x1 * scale_x)
		x2 = int(x2 * scale_x)
		y1 = int(y1 * scale_y)
		y2 = int(y2 * scale_y)
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
		if (x1 < 0)
			x1 = 0
		if (y1 < 0)
			y1 = 0
		url="http://igor2.repo.hu/tmp/pcblib/" tolower(T[t, "text"]) ".html"
		gsub(" ", "+", url)
		print "<area shape=\"rect\" coords=" q x1 "," y1 "," x2 "," y2 q " href=" q url q " alt=" q T[t, "text"] q ">"
		T[t, "text"] = "-"
	}
	print "</map>"
	print "</body></html>"
}

' < $pcb > map.html


