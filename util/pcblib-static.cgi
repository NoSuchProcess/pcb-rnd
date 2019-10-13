#!/bin/bash

ulimit -t 5
ulimit -v 80000

export HOME=/tmp
cd /tmp

# read the config
. /etc/pcblib.cgi.conf
CGI=$CGI_static

# import the lib
. $pcb_rnd_util/cgi_common.sh

find_fp()
{
	awk -v "fp=$QS_fp" -v "fpdir=$fpdir" '
		BEGIN {
			fp=tolower(fp)
		}
		($1 == "S") {
			n=tolower($2)
			sub("^.*/", "", n)
			sub(".fp$", "", n)
			sub(".ele$", "", n)
			if (n == fp) {
				print fpdir $2
				exit
			}
		}
	' < $sdir/cache
}

list_fps()
{
	find $fpdir | awk -v "fpdir=$fpdir" -v "CGI=$CGI" '
		/.svn/ { next }
		/parametric/ { next }

		{
			name=$0
			sub(fpdir, "", name)
			if (!(name ~ "/"))
				next
			dir=name
			fn=name
			sub("/.*", "", dir)
			sub("^[^/]*/", "", fn)
			vfn = fn
			sub(".fp$", "", vfn)

			LLEN[dir] += length(vfn)
			
			vfn = "<a href=" q CGI "?fp=" vfn q ">" vfn "</a>"
			
			if (LLEN[dir] > 8) {
				LLEN[dir] = 0
				sep = "<br>"
			}
			else
				sep = "&nbsp;"
			if (DIR[dir] != "")
				DIR[dir] = DIR[dir] sep vfn
			else
				DIR[dir] = vfn
		}

		END {
			print "<table border=1 cellpadding=20>"
			print "<tr>"
			for(n in DIR)
				print "<th>" n
			print "<tr>"
			for(n in DIR)
				print "<td valign=top>" DIR[n]
			print "</table>"
		}
	'
}

qs=`echo "$QUERY_STRING" | tr "&" "\n"`

for n in $qs
do
    exp="QS_$n"
    export $exp
done

export QS_cmd=`echo "$QS_cmd" | url_decode`

if test -z "$QS_fp"
then
	export QS_fp='TO220'
	gen=connector
fi

fn=`find_fp`
if test ! "$?" = 0
then
	echo "Content-type: text/plain"
	echo ""
	echo "Error: couldn't find $QS_fp"
	exit
fi

fptext=`cat $fn`
if test ! "$?" = 0
then
	echo "Content-type: text/plain"
	echo ""
	echo "Error reading footprint file for $QS_fp $fn"
	exit
fi

if test "$QS_output" = "text"
then
	echo "Content-type: text/plain"
	echo ""
	echo "$fptext"
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

	fptmp=`mktemp`
	$fp2preview --outfile $fptmp $cparm "$QS_fp" >/dev/null 2>&1
	cat $fptmp
	rm $fptmp

	exit
fi

echo "Content-type: text/html"
echo ""

echo "<html>"
echo "<body>"
echo "<H1> pcblib-static browse page </H1>"
echo '

<table border=1 cellpadding=10 cellspacing=0>
<tr><td bgcolor="lightgrey">
<i>
An outburst while looking for a powerjack connector <br>
footprint in the stock library: <br>
"What on earth is an <i>MSP430F1121 <u>footprint</u></i> and <br>
why do I need it in the default library?!"</i>
</table>

<br> &nbsp;<br> &nbsp;
<p>

<table border=0  cellpadding=10 cellspacing=10>
<tr>
<td valign=top width=40%>
<h3> pcblib </h3>
<p>
The default library in <a href="http://repo.hu/projects/pcb-rnd">pcb-rnd</a>
is called the "pcblib" (lib and newlib are already used by vanilla pcb).
Pcblib content consists of 
<a href="http://igor2.repo.hu/cgi-bin/pcblib-param.cgi">parametric (generated) footprints</a>
and static footprints ("file elements"). This page queries static footprints.
<p>
The goal of pcblib is to ship a minimalistic library of the real essential
parts, targeting small projects and hobbysts. Pcblib assumes users can
grow their own library by downloading footprints from various online sources
(e.g. <a href="http://gedasymbols.org"> gedasymbols</a>) or draw their own
as needed. Thus instead of trying to be a complete collection of footprints
ever drawn, it tries to collect the common base so that it is likely that 90%
of pcblib users will use 90% of the footprints in their projects.

<td> &nbsp;&nbsp;&nbsp;
<td valign=top bgcolor="#eefeee">
<h3>List of static footprints</h3>
<ul>
'


list_fps

echo '
</ul>
</table>
'


echo "<h2> Search and preview </h2>"

echo "<form action=\"$CGI\" method=get>"
echo "name: <input name=\"fp\" value=\"$QS_fp\">"
echo "<ul>"
echo "	<li><input type=\"checkbox\" name=\"mm\" value=\"1\" `checked $QS_mm`> draw grid in mm"
echo "	<li><input type=\"checkbox\" name=\"photo\" value=\"1\" `checked $QS_photo`> draw in \"photo mode\""
echo "</ul>"
echo "<p>"
echo "<input type=\"submit\" value=\"Find my footprint!\">"
echo "</form>"



QS_format=${QS_cmd##*_}
QS_fp_=${QS_cmd%%_*}

if test ! -z "$QS_fp"
then
echo "<h2> Result </h2>"
	echo "<h3> $QS_fp </H3>"

	echo "<table border=0>"
	echo "<tr><td valign=top>"
	echo "<img src=\"$CGI?$QUERY_STRING&output=png\">"

	echo "<td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"

	echo "<td valign=top>"
	echo "<pre>"
	echo "$fptext"
	echo "</pre>"
	echo "<p>Downloads:"
	echo "<br> <a href=\"$CGI?fp=$QS_fp&output=text\">footprint file</a>"
	echo "</table>"
fi


echo "</body>"
echo "</html>"
