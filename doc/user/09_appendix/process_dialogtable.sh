#!/bin/sh
# dumps a csv table of dialog behaviors into a simple html table
awk '
BEGIN {
	pcb_rnd_rev="pcb-rnd 1.2.3"
	FS = ","
	q="\""
	print "<!DOCTYPE HTML PUBLIC " q "-//W3C//DTD HTML 4.01 Transitional//EN" q " " q "http://www.w3.org/TR/html4/loose.dtd"  q ">"
	print "<html>"
	print "<head>"
	print "<title> pcb-rnd </title>"
	print "<meta http-equiv=" q "Content-Type" q " content=" q "text/html;charset=us-ascii" q ">"
	print "<link rel=" q "stylesheet" q " type=" q "text/css" q " href=" q " ./default.css" q ">"
	print "</head>"
	print "<body>"
	print "<p>"
	print "<table border=1>"
	print "<caption><b>"
	print  "pcb-rnd dialog behaviors" ", " pcb_rnd_rev
	print "</b>"
	getline
	getline
	i = 0
	while (i < NF ) {
	  i++
	  print "<th>"  $i "</th>"
	  }
	
}

!NF     {
	exit
}

// {
	printf "<tr><td> %s </td>\n", $1
	printf "<td> %s </td>\n", $2
	printf "<td> %s </td>\n", $3
	printf "<td> %s </td>\n", $4
	printf "<td> %s </td>\n", $5
	printf "<td> %s </td>\n", $6
}


END {
	print "</table>"
	print "</body>"
	print "</html>"
	getline
	print $0 "<br>"
}
'  pcb-rnd-dialogs
