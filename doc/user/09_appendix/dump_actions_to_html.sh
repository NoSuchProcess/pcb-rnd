#! /bin/bash
# bash script to dump pcb-rnd actions and parse output a html table for the
# docs
#
#errata: pcb_dump_actions function in src/hid_actions.c
	#printf("A%s\n", ca->action->name);
	#dump_string('D', desc);
	#dump_string('S', synt);

# we want to sort the pcb-rnd actions and process them into a captioned html
# table with three header columns: Action Name, Description, Syntax 
#
# if the line starts with "A", strip the letter and enclose the rest of the
# line in a <tr> tag
# if the next line starts with a D, strip the letter and enclose the rest of
# the line in a <td> tag, add newline and start the <td> tag for any syntax content
# any following lines that start with an S are printed in the <td> 
# repeat

#TODO: use pcb-rnd version used to generate the table in table header 
#TODO: fix escape parsing in syntax sections 

cat table_opener.html > action_reference.html 

(
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


