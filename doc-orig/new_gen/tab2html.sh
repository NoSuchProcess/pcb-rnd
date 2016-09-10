#!/bin/sh

# Configuration: $1 is the number of col pairs in a row

echo "<html><body><table border=1 cellspacing=0>"
awk -F "[\t]*" -v "cols=$1" '
BEGIN {
	len = 0
}

/^[@]/ {
# print header
	print "<tr>"
	for(n = 0; n < cols; n++)
		print "<th>" $1 "<th>" $2

	next
}

# load data
{
	CELL[len] = "<td>" $1 "<td>" $2
	len++
}

END {
	rows = len / cols
	if (rows != int(rows))
		rows++;
	rows = int(rows);
	for(r = 0; r < rows; r++) {
		print "<tr>"
		for(c = 0; c < cols; c++) {
			idx = c*rows + r
			cell = CELL[idx]
			if (cell == "")
				cell = "&nbsp;"
			print cell
		}
	}
}

'
echo "</table></body></html>"
