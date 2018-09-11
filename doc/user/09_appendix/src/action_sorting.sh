#!/bin/sh

# dumps pcb-rnd actions into a sorted simple html table for processing into docs 
#
#errata: pcb_dump_actions function in src/hid_actions.c
#        printf("A%s\n", ca->action->name);
#        dump_string('D', desc);
#        dump_string('S', synt);
(
	cd ../../../../src
	./pcb-rnd --dump-actions
) | awk '

function flush_sd()
{
		if ( a != "" ||  s != "" || d != "" ) {
			sub("^<br>", "", a)
			sub("^<br>", "", d)
			sub("^<br>", "", s)
			print  "<tr><td>" a "</td>" "<td>" d "</td>" "<td>" s "</td>"
			}
		
	a=""
	s=""
	d=""
}


/^A/ {
	flush_sd()
	sub("^A", "", $0)
	a = a "<br>" $0
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

' | sort -u
