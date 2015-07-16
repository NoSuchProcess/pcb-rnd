#!/bin/bash

ulimit -t 5
ulimit -v 80000

CGI=/cgi-bin/pcblib-param.cgi

gen()
{
	cd /home/igor2/C/pcb-rnd/pcblib-param/
	params=`echo $QS_cmd | sed "s/.*[(]//;s/[)]//" `
	./connector "$params"
}

radio()
{
	local chk
	if test "$3" = "$2"
	then
		chk=" checked=\"true\""
	fi
	echo "<input type=\"radio\" name=\"$1\" value=\"$2\"$chk>"
}

checked()
{
	if test ! -z "$1"
	then
		echo " checked=\"true\""
	fi
}

fix_ltgt()
{
	sed "s/</\&lt;/g;s/>/\&gt;/g"
}

qs=`echo "$QUERY_STRING" | tr "&" "\n"`

for n in $qs
do
    exp="QS_$n"
    export $exp
done

export QS_cmd=`echo "$QS_cmd" | /home/igor2/C/libporty/trunk/src/porty/c99tree/url.sh`

if test -z "$QS_cmd"
then
	export QS_cmd='connector(2,3)'
fi

if test "$QS_output" = "text"
then
	echo "Content-type: text/plain"
	echo ""
	gen
	exit
fi

if test "$QS_output" = "png"
then
	echo "Content-type: image/png"
	echo ""
	cparm=""
	if test ! -z "$QS_mm"
	then
		cparm="$cparm --mm"
	fi
	if test ! -z "$QS_photo"
	then
		cparm="$cparm --photo"
	fi
	(gen | /home/igor2/C/pcb-rnd/util/fp2anim $cparm; echo 'screenshot "/dev/stdout"') | /usr/local/bin/animator -H
	exit
fi

echo "Content-type: text/html"
echo ""

echo "<html>"
echo "<body>"
echo "<H1> pcblib-param test page </H1>"
echo '

<table border=1 cellpadding=10 cellspacing=0>
<tr><td bgcolor="lightgrey">
<i>
"Do you remember the good old days? When I was
<br> young, I said <b>footprint=CONNECTOR 2 13</b>
<br> in gschem and got a 2*13 pin connector in PCB."</i>
</table>

<br> &nbsp;<br> &nbsp;
<p>

<table border=0  cellpadding=10 cellspacing=10>
<tr>
<td valign=top width=40%>
<h3> Syntax </h3>
<p>
In <a href="http://repo.hu/projects/pcb-rnd">pcb-rnd</a>,
automatic footprint generation on the fly is called "parametric
footprints". It is a smaller and more generic infrastructure compared
to the m4 footprints in pcb. The goal was to fix three problems.
<ul>
	<li> 1. Languages: m4 should not be mandatory for footprint generators - the user should be free to choose any language
	<li> 2. Simplicity: references to m4 should not be hardwired in pcb, gsch2pcb and gsch2pcb.scm, but footprint generation should be generic, transparent and external
	<li> 3. Unambiguity: gsch2pcb should not be guessing whether to use file elements or call a generator. Instead of complex heuristics, there should eb a simple, distinct syntax for parametric footprints.
</ul>
<p>
The new syntax is that if a footprint attribute contains parenthesis, it is
a parametric footprint, else it is a file footprint. Parametric footprints
are sort of a function call. The actual syntax is similar to the on in
<a href="http://www.openscad.org"> openscad </a>:
parameters are separated by commas, and they are either positional (value)
or named (name=value).
<p>
For exmaple a simple pin-grid
connector has the "prototype" of <i>connector(nx, ny, spacing)</i>, where
nx and ny are the number of pins needed in X and Y directions, spacing is
the distance between pin centers, with a default of 100 mil. The following
calls will output the same 2*3 pin, 100 mil footprint:
<ul>
	<li> connector(2, 3)
	<li> connector(2, 3, 100)
	<li> connector(nx=2, ny=3, spacing=100)
	<li> connector(spacing=100, nx=2, ny=3)
	<li> connector(ny=3, spacing=100, nx=2)
</ul>

<td> &nbsp;&nbsp;&nbsp;
<td valign=top bgcolor="#eefeee">
<h3>Generators available</h3>
<ul>
	<li> connector()
</ul>
<P>
TODO
</table>
'


echo "<h2> Try it online </h2>"
echo "<b>source:</b>"

echo "<form action=\"$CGI\" method=get>"
echo "<textarea id=\"cmd\" name=\"cmd\" cols=80 rows=3>$QS_cmd</textarea>"
echo "<ul>"
echo "	<li><input type=\"checkbox\" name=\"mm\" value=\"1\" `checked $QS_mm`> draw grid in mm"
echo "	<li><input type=\"checkbox\" name=\"photo\" value=\"1\" `checked $QS_photo`> draw in \"photo mode\""
echo "</ul>"
echo "<p>"
echo "<input type=\"submit\" value=\"Generate my footprint!\">"
echo "</form>"



QS_format=${QS_cmd##*_}
QS_cmd_=${QS_cmd%%_*}

if test ! -z "$QS_cmd"
then
echo "<h2> Result </h2>"
	echo "<h3> $QS_cmd </H3>"

	echo "<table border=0>"
	echo "<tr><td valign=top>"
	echo "<img src=\"$CGI?$QUERY_STRING&output=png\">"

	echo "<td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"

	echo "<td valign=top>"
	echo "<pre>"
	gen
	echo "</pre>"
	echo "<p>Downloads:"
	echo "<br> <a href=\"$CGI?$QUERY_STRING&output=text\">footprint file</a>"
	echo "</table>"
fi


echo "</body>"
echo "</html>"
