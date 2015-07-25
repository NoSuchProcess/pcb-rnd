#!/bin/bash

ulimit -t 5
ulimit -v 80000

CGI=/cgi-bin/pcblib-param.cgi
gendir=/home/igor2/C/pcb-rnd/pcblib/parametric/

gen()
{
	cd $gendir
	params=`echo $QS_cmd | sed "s/.*[(]//;s/[)]//" `
	./$gen "$params"
}

help_params()
{
awk  -v "CGI=$CGI" "$@" '
BEGIN {
	prm=0
	q="\""
}

/@@include/ {
	print $2 > "/dev/stderr"
}

(match($0, "@@desc") || match($0, "@@params") || match($0, "@@purpose") || match($0, "@@example")) {
	txt=$0
	key=substr($0, RSTART, RLENGTH)
	sub(".*" key "[ \t]*", "", txt)
	HELP[key] = HELP[key] " " txt
	next
}

/@@param:/ {
	sub(".*@@param:", "", $0)
	p=$1
	sub(p "[\t ]*", "", $0)
	PARAM[prm] = p 
	PDESC[prm] = $0
	prm++
	next
}

# build PPROP[paramname,propname], e.g. PPROP[pin_mask, dim]=1
/@@optional:/ || /@@dim:/ {
	key = $0
	sub("^.*@@", "", key)
	val = key
	sub(":.*", "", key)
	sub("^[^:]*:", "", val)
	PPROP[val,key] = 1
	next
}

# build PDATA[paramname,propname], e.g. PDATA[pin_mask, default]=123
/@@default:/ {
	key = $1
	sub("^" key "[ \t]*", "", $0)
	sub("^.*@@", "", key)
	val = key
	sub(":.*", "", key)
	sub("^[^:]*:", "", val)
	PDATA[val,key] = $0
	next
}

# build:
#  PDATAK[paramname,n]=key (name of the value)
#  PDATAV[paramname,n]=description (of the given value)
#  PDATAN[paramname]=n number of parameter values
/@@enum:/ {
	key = $1
	sub("^" key "[ \t]*", "", $0)
	sub("^.*@@enum:", "", key)
	val = key
	sub(":.*", "", key)
	sub("^[^:]*:", "", val)
	idx = int(PDATAN[key])
	PDATAK[key,idx] = val
	PDATAV[key,idx] = $0
	PDATAN[key]++
	next
}

END {
	if (header) {
		print "<b>" HELP["@@purpose"] "</b>"
		print "<p>"
		print HELP["@@desc"]
	}

	if (content)
		print "<h4>" content "</h4>"

	if ((print_params) && (HELP["@@params"] != ""))
		print "<p>Ordered list (positions): " HELP["@@params"]


	if (content) {
		print "<table border=1 cellspacing=0 cellpadding=10>"
		print "<tr><th align=left>name <th align=left>man<br>dat<br>ory<th align=left>description <th align=left>value (bold: default)"
		for(p = 0; p < prm; p++) {
			name=PARAM[p]
			print "<tr><td>" name
			if (PPROP[name, "optional"])
				print "<td> &nbsp;"
			else
				print "<td> yes"
			print "<td>" PDESC[p]
			print "<td>"
			if (PDATAN[name] > 0) {
				print "<table border=1>"
				for(v = 0; v < PDATAN[name]; v++) {
					print "<tr><td>"
					isdef = (PDATA[name, "default"] == PDATAK[name, v])
					if (isdef)
						print "<b>"
					print PDATAK[name, v]
					if (isdef)
						print "</b>"

					print "<td>" PDATAV[name, v]
				}
				print "</table>"
			}
			if (PPROP[name, "dim"]) {
				print "Dimension: a number with an optional unit (mm or mil, default is mil)"
				if (PDATA[name, "default"] != "")
				print "<br>Default: <b>" PDATA[name, "default"] "</b>"
			}
		}
		print "</table>"
	}

	if (footer) {
		print "<h3>Example</h3>"
		print "<a href=" q CGI "?cmd=" HELP["@@example"] q ">"
		print HELP["@@example"]
		print "</a>"
		print "</body></html>"
	}
}
'
}

help()
{
	local incl n
	echo "
<html>
<body>
<h1> pcblib-param help for $QS_cmd() </h1>
"

	incl=`tempfile`


	help_params -v "header=1" -v "content=$gen parameters" -v "print_params=1" < $gendir/$gen 2>$incl

	for n in `cat $incl`
	do
		help_params -v "content=parameters (by $n)" < $gendir/$n 2>/dev/null
	done

	rm $incl

	help_params -v "footer=1" < $gendir/$gen 2>/dev/null

}

error()
{
	echo "Content-type: text/plain"
	echo ""
	echo "Error: $*"
	exit 0
}

list_gens()
{
	awk -v "CGI=$CGI" '
		BEGIN {
			q="\""
		}
		/@@purpose/ {
			sub(".*@@purpose", "", $0)
			PURPOSE[FILENAME] = PURPOSE[FILENAME] $0
			next
		}
		/@@example/ {
			sub(".*@@example", "", $0)
			EXAMPLE[FILENAME] = EXAMPLE[FILENAME] $0
			next
		}

		/@@params/ {
			sub(".*@@params", "", $0)
			PARAMS[FILENAME] = PARAMS[FILENAME] $0
			next
		}
		
		END {
			for(fn in PURPOSE) {
				gn=fn
				sub(".*/", "", gn)
				params=PARAMS[fn]
				sub("^[ \t]*", "", params)
				sub("[ \t]*$", "", params)
				print "<li> <a href=" q CGI "?cmd=" EXAMPLE[fn] q ">" gn "(<small>" params "</small>) </a> - " PURPOSE[fn]
				print "- <a href=" q CGI "?cmd=" gn "&output=help"  q "> HELP </a>"
			}
		}
	' $gendir/*
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
	export QS_diamond=1
	gen=connector
else
	gen=`awk -v "n=$QS_cmd" '
	BEGIN {
		sub("[(].*", "", n)
		gsub("[^a-zA-Z0-9_]", "", n)
		print n
	}
	'`
	
	if test -z `grep "@@purpose" $gendir/$gen`
	then
		error "Invalid generator \"$gen\" (file)"
	fi
fi


if test "$QS_output" = "help"
then
	echo "Content-type: text/html"
	echo ""
	help
	exit
fi

fptext=`gen`
if test ! "$?" = 0
then
	echo "Content-type: text/plain"
	echo ""
	echo "Error generating the footprint:"
	gen 2>&1 | grep -i error
	exit
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
	if test ! -z "$QS_diamond"
	then
		cparm="$cparm --diamond"
	fi
	if test ! -z "$QS_photo"
	then
		cparm="$cparm --photo"
	fi
	(echo "$fptext" | /home/igor2/C/pcb-rnd/util/fp2anim $cparm; echo 'screenshot "/dev/stdout"') | /usr/local/bin/animator -H
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
'
list_gens
echo '
</ul>
</table>
'


echo "<h2> Try it online </h2>"
echo "<b>source:</b>"

echo "<form action=\"$CGI\" method=get>"
echo "<textarea id=\"cmd\" name=\"cmd\" cols=80 rows=3>$QS_cmd</textarea>"
echo "<ul>"
echo "	<li><input type=\"checkbox\" name=\"mm\" value=\"1\" `checked $QS_mm`> draw grid in mm"
echo "	<li><input type=\"checkbox\" name=\"photo\" value=\"1\" `checked $QS_photo`> draw in \"photo mode\""
echo "	<li><input type=\"checkbox\" name=\"diamond\" value=\"1\" `checked $QS_diamond`> diamond at 0;0"
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
	echo "$fptext"
	echo "</pre>"
	echo "<p>Downloads:"
	echo "<br> <a href=\"$CGI?$QUERY_STRING&output=text\">footprint file</a>"
	echo "</table>"
fi


echo "</body>"
echo "</html>"
