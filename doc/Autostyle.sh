#!/bin/sh

autostyle()
{
	awk -v "template=$1" '
	BEGIN {
		while((getline < template) > 0) {
			if (parse_auto(RES, $0)) {
				if (RES["action"] == "begin")
					curr = RES["ID"]
				else
					reset_curr = 1
			}
			if (curr != "")
				AUTO[curr] = AUTO[curr] $0 "\n"
			if (reset_curr) {
				curr = ""
				reset_curr = 0
			}
		}
	}

	function parse_auto(RES, line     ,tmp)
	{
		if (!(line ~ "<!--AUTO"))
			return 0
		sub(".*<!--AUTO[ \t]*", "", line)
		sub("[ \t]*-->.*", "", line)
		line = tolower(line)
		tmp = line
		sub("[ \t].*$", "", tmp)
		RES["ID"] = tmp
		tmp = line
		sub("^[^ \t]*[ \t]*", "", tmp)
		RES["action"] = tmp
		return 1
	}

	{
		if (parse_auto(RES, $0)) {
			if (RES["action"] == "begin")
				skip = 1
			else if (RES["action"] == "end") {
				printf("%s", AUTO[RES["ID"]])
				skip = 0
			}
			next
		}
	}

	(!skip) { print $0 }

	'
}

for html in $*
do
	case $html in
		Autostyle.html) ;;
		*)
			mv $html $html.tmp
			autostyle "Autostyle.html" < $html.tmp > $html
			if test $? = 0
			then
				rm $html.tmp
			else
				echo "Failed on $html, keeping the original version."
				mv $html.tmp $html
			fi
	esac
done
