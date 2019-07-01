#!/bin/sh

# collates the pcb-rnd action table into a html doc page

asrc="../action_src"

cd ../../../../src
pcb_rnd_ver=`./pcb-rnd --version`
pcb_rnd_rev=`svn info ^/ | awk '/Revision:/ {
	print $0
	got_rev=1
	exit
	}
	END {
		if (!got_rev)
		print "Rev unknown"
	}
	'`
cd ../doc/user/09_appendix/src

echo 	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">
<html>
<head>
<title> pcb-rnd user manual </title>
<meta http-equiv=\"Content-Type\" content=\"text/html;charset=us-ascii\">
<link rel=\"stylesheet\" type=\"text/css\" href=\" ../default.css\">
</head>
<body>
<p>
<h1> pcb-rnd User Manual: Appendix </h1>
<p>
<h2> Action Reference</h2>
<table border=1>"
echo "<caption>\n" "<b>"
echo $pcb_rnd_ver ", " $pcb_rnd_rev
echo "</b>"
echo  "<th> Action <th> Description <th> Syntax <th> Plugin"
(
	cd ../../../../src
	./pcb-rnd --dump-actions 2>/dev/null
) | awk '

function flush_sd()
{
		if ( a != "" ||  s != "" || d != "" ) {
			sub("^<br>", "", a)
			sub("^<br>", "", d)
			sub("^<br>", "", s)
			sub("^<br>", "", c)
			print  "<tr><act>" a "</act>" "<td>" d "</td>" "<td>" s "</td>" "<td>" c "</td>"
			}
		
	a=""
	s=""
	d=""
	c=""
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

/^C/ {
	sub("^C", "", $0)
	c = c "<br>" $0
	next
}

' | sort -fu | awk -v "asrc=$asrc" '
# insert links around actions where applicable
	BEGIN {
		q = "\""
	}
	/<act>/ {
		pre = $0
		sub("<act>.*", "", pre)
		post = $0
		sub(".*</act>", "", post)
		act = $0
		sub(".*<act>", "", act)
		sub("</act>.*", "", act)
		loact = tolower(act)
		fn = asrc "/" loact ".html"
		if ((getline < fn) == 1)
			print pre "<td><a href=" q "action_details.html#" loact q ">" act "</a></td>" post
		else
			print pre "<td>" act "</td>" post
		close(fn)
		next
	}

	{ print $0 }
	
	END {
		print "</table>"
		print "</body>"
		print "</html>"
	}
'
