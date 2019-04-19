#!/bin/sh

trunk=../../../..

# exceptions
dlgtbl='
BEGIN {
	# if source filename is the index and value regex-matches either id or name
	# just ignore that line
	IGNORE["src_plugins/dialogs/dlg_view.c"] = "<dyn>"
	IGNORE["src_plugins/dialogs/act_dad.c"] = "<dyn>"
}

END {
	out("view*", "view*", "src_plugins/dialogs/dlg_view.c")
}
'

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
<h2> List of GUI dialog boxes </h2>
<table>
<tr>
	<th> ID
	<th> dialog box name
	<th> action
	<th> source
'

$trunk/util/devhelpers/list_dialogs.sh | awk -F "[\t]" '
function orna(s)
{
	if ((s == "") || (s == "<dyn>")) return "n/a"
	return s
}

'"$dlgtbl"'
'"`cat dialog_extra.awk`"'

function out(id, name, src, action) {
	if (action == "") {
		if (id in ACTION) action = ACTION[id]
		else if (src in ACTION) action = ACTION[src]
	}
	print "<tr><td>" orna(id) "<td>" orna(name) "<td>" orna(action) "<td>" src
}

{
	id=$1
	name=$2
	src=$3
	if ((src in IGNORE) && ((name ~ IGNORE[src]) || (id ~ IGNORE[src])))
		next
	out(id, name, src)
}
'

echo '
</table>
</body>
</html>
'
