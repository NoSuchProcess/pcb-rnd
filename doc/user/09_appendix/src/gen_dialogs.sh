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
{
	print "<tr><td>" $1 "<td>" $2 "<td>" $3
}
'

echo '
</table>
</body>
</html>
'
