#!/bin/sh

# dumps pcb-rnd actions into a simple html table for the docs 
#
#errata: pcb_dump_actions function in src/hid_actions.c
#        printf("A%s\n", ca->action->name);
#        dump_string('D', desc);
#        dump_string('S', synt);

cat table_opener.html > action_reference.html 

(
	cd ../../../src
	pcb-rnd --version
	pcb-rnd --dump-actions
) | awk '

function flush_sd()
{
	if (s != "") {
		sub("^<br>", "", d)
		sub("^<br>", "", s)
		print "<td>" d "</td>"
		print "<td>" s "</td>"
	}
	s=""
	d=""
}

BEGIN {
# first line of input is the version
	getline pcb_rnd_ver

	print "some headers here..."
	print pcb_rnd_ver
}

/^A/ {
	flush_sd()
	sub("^A", "", $0)
	printf "<tr><td> %s </td>\n", $0
	next
}

/^D"/ {
	sub("^D", "", $0)
	d = d "<br>" $0
	next
}

/^S/ {
	sub("^S", "", $0)
	s = s "<br>" $0
	next
}

END {
	flush_sd()
}

' >> action_reference.html


