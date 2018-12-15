#!/bin/sh

ROOT=../..
fn=`pwd`/$1
cd $ROOT/src

echo '
DRC(dump)
' | ./pcb-rnd $fn -c rc/quiet=1 -c rc/library_search_paths= --gui batch | awk '
/^V/ { print ""; next}
/^E/ {  next}
{
	line=$0
	chr=substr(line, 1, 1)
	sub("^.", "", line)
	print chr, line
}

'