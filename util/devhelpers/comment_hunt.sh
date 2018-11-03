#!/bin/sh

# Hunt for comments that are longer than $1 in all file names specified after
# $1. Useful for detecting documentation that should be migrated from code to
# doc/

hunt()
{
awk  -v "THR=$1" -v "fn=$2" '
(open) { comment = comment $0 "\n"; cnt++ }

/\/[*]/ && (!open) {
	open=1
	cnt=1
	comment=$0 "\n"
}

/[*]\// && (open) {
	if ((cnt > THR) && (!(comment ~ "MERCHANTABILITY")))
		print fn "::" cnt "::" comment
	open=0
	cnt=0
	comment=""
}

' < $2
}

THR=$1
shift 1
for n in $*
do
	hunt $THR $n
done
