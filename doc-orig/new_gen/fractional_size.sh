#!/bin/sh

echo "<html><body><table border=1 cellspacing=0>"
awk '
BEGIN {
# Configuration: number of col pairs in a row
	cols = 4

# print header
	print "<tr>"
	for(n = 0; n < cols; n++)
		print "<th> Drill<br>Size <th> Diameter<br>(inches)"
	print "<tr>"
	len = 0
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
