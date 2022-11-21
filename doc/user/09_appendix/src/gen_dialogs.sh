#!/bin/sh

trunk=../../../..

if test -z "$LIBRND_LIBDIR"
then
# when not run from the Makefile
	LIBRND_LIBDIR=`cd $trunk/doc/developer/packaging && make -f librnd_root.mk libdir`
fi

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
'

dlgextra="`cat dialog_extra.awk`"

. $LIBRND_LIBDIR/devhelpers/list_dialogs.sh

print_hdr
list_dlgs $trunk/src/*.c $trunk/src_plugins/*/*.c | gen_html

echo '
</table>
</body>
</html>
'
