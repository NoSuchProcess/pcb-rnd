#!/bin/sh

path=../../src_plugins

sloc()
{
	(cd "$1" && sloccount .) | awk '/^Total Phys/ { size=$9; sub(",", "", size); print size }'
}

(
cat pre.html
for n in $path/*
do
	if test -d "$n"
	then
		echo $n >&2
		bn=`basename $n`
		echo "<tr><th>$bn<td>"
		sloc $n
		awk '
			/^#/ {
				key=$1
				sub("#", "", key)
				sub("[:=]", "", key)
				$1=""
				DB[key]=$0
				next
			}
			{ desc = desc " " $0 }

			function strip(s) {
				sub("^[ \t]*", "", s)
				sub("[ \t]*$", "", s)
				return s
			}

			END {
				st = DB["state"]
				if (st == "works")
					clr = "bgcolor=\"green\""
				else if (st ~ "partial")
					clr = "bgcolor=\"yellow\""
				else if ((st ~ "fail") || (st ~ "disable"))
					clr = "bgcolor=\"red\""

				print "<td  " clr " >" st
				if (DB["lstate"] != "")
					print "<br> (" strip(DB["lstate"]) ")"
				print "<td>" DB["default"]
				if (DB["ldefault"] != "")
					print "<br> (" strip(DB["ldefault"]) ")"
				print "<td>" desc
			}
		' < $n/README
	fi
done
cat post.html
) > index.html
