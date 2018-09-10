#!/bin/sh

PLUGINS=../../../../src_plugins

echo '
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
	<title> pcb-rnd - list of file formats </title>
	<meta http-equiv="Content-Type" content="text/html;charset=us-ascii">
	<link rel="stylesheet" type="text/css" href="../default.css">
</head>
<body>

<h1> pcb-rnd User Manual: Appendix </h1>
<p>
<h2> File format support </h2>

<table border=1>
	<tr>
	<th> plugin <th> native <th> state <th> reads formats <th> writes formats
'

files_io=`ls $PLUGINS/io_*/*.pup | sort`
files_e=`ls $PLUGINS/export_*/*.pup | sort`
files_i=`ls $PLUGINS/import_*/*.pup | sort`

for n in $files_io $files_i $files_e
do
	bn=`basename $n`
	bn=${bn%%.pup}
	sed '
		/^[$]fmt[-]/ { s@^@'$bn' @; p }
		/^[$]state/ { s@^@'$bn' @; p }
		{ d }
	' $n
done | awk '
	BEGIN {
		plgs = 0
	}
	{
		if ($1 != last) {
			PLG[plgs] = $1
			last = $1
			plgs++
		}
		p=$1
		f=$2
		text=$0
		sub("^[^ \t]+[ \t]+[^ \t]+[ \t]+", "", text)
		DATA[p, f] = DATA[p, f] SUBSEP text
	}

	function col(mask    ,n,v,i,A) {
		print " <td>"
		v = split(DATA[mask], A, "[" SUBSEP "]")
		if (v == 0) {
			print "n/a"
			return
		}
		i = 0;
		for(n = 1; n <= v; n++) {
			if (A[n] == "")
				continue;
			if (i)
				print "  <br>"
			print "  " A[n]
			i++
		}
	}

	function strip(s)
	{
		gsub("[" SUBSEP "]", " ", s)
		sub("^[ \t]*", "", s)
		sub("[ \t]*$", "", s)
		return s
	}

	function row(plg) {
		print "<tr><th>" plg
		print "<td>" strip(DATA[plg, "$fmt-native"])
		print "<td>" strip(DATA[plg, "$state"])
		col(plg SUBSEP "$fmt-feature-r")
		col(plg SUBSEP "$fmt-feature-w")
	}

	END {
		for(p=0; p < plgs; p++)
			if (DATA[PLG[p], "$fmt-native"] ~ "yes")
				row(PLG[p])

		for(p=0; p < plgs; p++)
			if (!(DATA[PLG[p], "$fmt-native"] ~ "yes"))
				row(PLG[p])
	}
'


echo '
</table>
</body>
</html>
'
