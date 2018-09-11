#!/bin/sh
(
	cd ../../../../src
	./pcb-rnd --dump-actions
) | awk '

function flush_sd()
{
	if (a != "" ){
		if ( s != "" || d != "" ) {
			sub("^<br>", "", d)
			sub("^<br>", "", s)
			print  "<tr><td>" a "</td>" "<td>" d "</td>" "<td>" s "</td>"
			}
	}
	a=""
	s=""
	d=""
}


/^A/ {
	flush_sd()
	sub("^A", "", $0)
	a = a $0
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
