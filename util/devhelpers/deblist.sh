#!/bin/sh

# Process the debian/control file and produce a html table that explains
# the purpose of each package

echo '
<html>
<head>
	<title> pcb-rnd - Debian package list </title>
<!--AUTO head begin-->

<!--AUTO head end-->
</head>
<body>

<!--AUTO navbar begin-->

<!--AUTO navbar end-->

<h1> pcb-rnd - Debian package list </h1>
<table border=1>
<tr><th>name <th> internal dependencies <th> description
'

awk '
function out()
{
	if (pkg == "")
		return
	print "<tr><td>" pkg "<td>" deps "<td>" desc
	pkg=""
	deps=""
	desc=""
	para=0
}

/^Package:/ {
	out();
	pkg=$2
	para=0
}
#Depends: ${misc:Depends}, ${shlibs:Depends}, pcb-rnd-core, pcb-rnd-gtk
/^Depends:/ {
	deps=$0
	sub("Depends: *", "", deps)
	gsub("[$][{][^}]*[}],?", "", deps)
}

/^ [.]/ {
	if (para > 0)
		desc = desc "<p>"
	para++
	next
}

(para > 0) {
	desc = desc " " $0
}

END {
	out()
}
'


echo '
</table>
'