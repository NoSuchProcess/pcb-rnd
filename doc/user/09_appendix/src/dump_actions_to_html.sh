#!/bin/sh

# dumps pcb-rnd actions into a simple html table for the docs 
#
#errata: pcb_dump_actions function in src/hid_actions.c
#        printf("A%s\n", ca->action->name);
#        dump_string('D', desc);
#        dump_string('S', synt);


(
	cd ../../../../src
	./pcb-rnd --version
	svn info ^/ | awk '/Revision:/ {
		print $0
		got_rev=1
		exit
		}
		END {
			if (!got_rev)
			print "Rev unknown"
		}
		'
	./pcb-rnd --dump-actions
) | awk '

function flush_sd()
{
	if (s != "" || d != "" ) {
		sub("^<br>", "", d)
		sub("^<br>", "", s)
		print "<td>" d "</td>"
		print "<td>" s "</td>"
	}
	s=""
	d=""
}

BEGIN {
	q="\""
# first line of input is the version
	getline pcb_rnd_ver
	getline pcb_rnd_rev
	print "<!DOCTYPE HTML PUBLIC " q "-//W3C//DTD HTML 4.01 Transitional//EN" q " " q "http://www.w3.org/TR/html4/loose.dtd"  q ">"
	print "<html>"
	print "<head>"
	print "<title> pcb-rnd user manual </title>"
	print "<meta http-equiv=" q "Content-Type" q " content=" q "text/html;charset=us-ascii" q ">"
	print "<link rel=" q "stylesheet" q " type=" q "text/css" q " href=" q " ../default.css" q ">"
	print "</head>"
	print "<body>"
	print "<p>"
        print "<h1> pcb-rnd User Manual: Appendix </h1>"
	print "<p>"
	print "<h2> Pcb-rnd Action Reference</h2>"
	print "<table border=1>"
	print "<caption><b>"
	print  pcb_rnd_ver ", " pcb_rnd_rev
	print "</b>"
	print "<th>Action<th> Description <th> Syntax"
}

/^A/ {
	flush_sd()
	sub("^A", "", $0)
	printf "<tr><td> %s </td>\n", $0
	next
}

/^D/ {
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
	print "</table>"
	print "</body>"
	print "</html>"

}

' 


