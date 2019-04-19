#!/bin/sh

trunk=../../../..

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
	<th> source
'

$trunk/util/devhelpers/list_dialogs.sh | awk -F "[\t]" '
function orna(s)
{
	if ((s == "") || (s == "<dyn>")) return "n/a"
	return s
}
{
	id=$1
	name=$2
	print "<tr><td>" orna(id) "<td>" orna(name) "<td>" $3
}
'

echo '
</table>
</body>
</html>
'
