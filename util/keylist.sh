#!/bin/sh
# parse the .res files for GUI HIDs and build a table of keys

if test -z "$*"
then
	echo ""
	echo "$0: Generate a html table from pcb menu res files."
	echo "Usage: $0 file1 [file2 [file3 ... [fileN]]]"
	echo ""
	exit
else
	res_files="$@"
fi

# split the file into one token per line using a state machine
# tokens are:
#  - quoted strings ("")
#  - control characters: {,  }, =
#  - other words
tokenize()
{
	awk '
		BEGIN {
			q = "\""
		}
		function echo(c) {
			had_nl = (c == "\n")
			printf "%s", c
		}
		function nl() {
			if (!had_nl)
				echo("\n")
		}
		function parse(c) {
			if (in_comment)
				return
			if (in_quote) {
				if (bslash) {
					echo(c)
					bslash = 0
					return
				}
				if (c == "\\")
					bslash = 1
				if (c == q) {
					nl();
					in_quote = 0
					return
				}
				echo(c)
				return
			}
			if (c == "#") {
				in_comment = 1
				return
			}
			if (c == q) {
				in_quote = 1
				echo(c)
				return
			}
			if (c ~ "[ \t]") {
				nl()
				return
			}
			if (c ~ "[{}=]") {
				nl()
				echo(c)
				nl()
				return
			}
			echo(c)
		}
	
		{
			in_comment = 0
			l = length($0)
			for(n = 1; n <= l; n++)
				parse(substr($0, n, 1))
		}
	'
}

extract_from_res()
{
	tokenize | awk -v "src=$1" '
		BEGIN {
			sub(".*/", "", src)
		}
		/^=$/ {
			if (last != "a") {
				last = ""
				getline t
				next
			}
			last = ""

			getline t
			if (t != "{")
				next
			getline k1
			getline k2
			getline t
			if (t != "}")
				next
			getline action
			if (action == "}")
				action = last_act
			sub("^\"", "", k2)
			sub("\"$", "", k2)
			gsub(" \t", "", k2)
			split(tolower(k2), K, "<key>")
			if (K[1] != "") {
				mods = ""
				if (K[1] ~ "alt")   mods = mods "-alt"
				if (K[1] ~ "ctrl")  mods = mods "-ctrl"
				if (K[1] ~ "shift") mods = mods "-shift"
			}
			else
				mods = ""
			print K[2] mods, src, action
			last_act = ""
			next
		}
		/[a-zA-Z]/ {
			if (last != "")
				last_act = last
		}
		{ last = $0 }
	'
}


gen_html()
{
	awk '
	BEGIN {
		HIDNAMES["gpcb-menu.res"] = "gtk"
		HIDNAMES["pcb-menu.res"]  = "lesstif"
	}
	function to_base_key(combo)
	{
		sub("-.*", "", combo)
		return combo
	}

	function to_mods(combo)
	{
		if (!(combo ~ ".-"))
			return ""
		sub("^[^-]*[-]", "", combo)
		return combo
	}

	{
		ROWSPAN[to_base_key($1)]++
		LIST[NR] = $1
		ACTION[$2, $1] = $3
		HIDS[$2]++
	}

	END {
		print "<html><body>"
		print "<h1> Key to action bindings </h1>"
		print "<table border=1>"
		printf("<tr><th> key <th> modifiers")
		for(h in HIDS)
			printf(" <th>%s<br>%s", h, HIDNAMES[h])
		print ""
		for(n = 1; n <= NR; n++) {
			key = LIST[n]
			base = to_base_key(key)
			mods = to_mods(key)
			print "<tr>"
			if (base != last_base)
				print "	<td rowspan=" ROWSPAN[base] ">" base
			print "	<td>" mods
			for(h in HIDS)
				print "	<td>", ACTION[h, key]
			last_base = base;
		}
		print "</table>"
		print "</body></html>"
	}
	'
}

for n in $res_files 
do
	extract_from_res "$n" < $n
done | sort | gen_html

