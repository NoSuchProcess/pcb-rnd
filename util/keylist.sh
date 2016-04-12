#!/bin/sh
#   keylist - list hotkey->actions found in .lht files in a html table, per HID
#   Copyright (C) 2015..2016 Tibor 'Igor2' Palinkas
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#   http://repo.hu/projects/pcb-rnd

AWK=awk

export LANG=C

need_lht()
{
	echo "lhtflat not found. Please install lihata to run this script. The" >&2
	echo "svn extern in src_3rd/ is not enough, this script requires a full" >&2
	echo "installation from svn://repo.hu/lihata/trunk" >&2
	exit 1
}

# make sure lhtflat is installed
echo "" | lhtflat || need_lht


if test -z "$*"
then
	echo ""
	echo "$0: Generate a html table from pcb menu lht files."
	echo "Usage: $0 file1 [file2 [file3 ... [fileN]]]"
	echo ""
	exit
else
	res_files="$@"
fi

extract_from_lht()
{
	lhtflat | $AWK -F '[\t]' -v "fn=$1" '

#data	text	//main_menu/1::Edit/submenu/11::Edit name of/submenu/1::pin on layout/a	Shift Ctrl<Key>n
#data	text	//main_menu/1::Edit/submenu/11::Edit name of/submenu/1::pin on layout/action	ChangeName(Object, Number) 

	{
		parent=$3
		sub("/[^/]*$","", parent)
		node=$3
		sub("^.*/","", node)
	}

	(($1 == "data") && ($2 == "text")) {
		# simple text node: accel key
		if (node == "a") {
			split(tolower($4), K, "<key>")
			if (K[1] != "") {
				mods = ""
				if (K[1] ~ "alt")   mods = mods "-alt"
				if (K[1] ~ "ctrl")  mods = mods "-ctrl"
				if (K[1] ~ "shift") mods = mods "-shift"
			}
			else
				mods = ""

			KEY[parent] = K[2] mods
		}

		# simple text node: action
		if (node == "action")
			ACTION[parent] = $4

		# list item: action
		if ($3 ~ "/action/[0-9]+::$") {
			parent = $3
			sub("/action/[^/]*$", "", parent)
			if (ACTION[parent] != "")
				ACTION[parent] = ACTION[parent] ";" $4 
			else
				ACTION[parent] = $4 
		}
	}

	END {
		for(n in KEY)
			print KEY[n] "\t" fn "\t" ACTION[n]
	}
	'
}

# convert a "key src action" to a html table with rowspans for base keys
gen_html()
{
	$AWK -F '[\t]' '
	BEGIN {
		HIDNAMES["gpcb-menu.res"] = "gtk"
		HIDNAMES["pcb-menu.res"]  = "lesstif"
		CLR[0] = "#FFFFFF"
		CLR[1] = "#DDFFFF"
		key_combos = 0
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
		if (last != $1) {
			LIST[key_combos++] = $1
			ROWSPAN[to_base_key($1)]++
		}
		ACTION[$2, $1] = $3
		HIDS[$2]++
		last = $1
	}

	END {
		print "<html><body>"
		print "<h1> Key to action bindings </h1>"
		print "<table border=1 cellspacing=0>"
		printf("<tr><th> key <th> modifiers")
		colspan = 2
		for(h in HIDS) {
			printf(" <th>%s<br>%s", h, HIDNAMES[h])
			colspan++
		}
		print ""
		for(n = 0; n < key_combos; n++) {
			key = LIST[n]
			base = to_base_key(key)
			mods = to_mods(key)
			if (base != last_base)
				clr_cnt++
			print "<tr bgcolor=" CLR[clr_cnt % 2] ">"
			if (base != last_base)
				print "	<th rowspan=" ROWSPAN[base] ">" base
			if (mods == "")
				mods = "&nbsp;"
			print "	<td>" mods
			for(h in HIDS) {
				act = ACTION[h, key]
				if (act == "")
					act = "&nbsp;"
				else
					gsub(");", "); ", act)
				print "	<td>", act
			}
			last_base = base
		}
		print "</table>"
		print "</body></html>"
	}
	'
}

for n in $res_files 
do
	extract_from_lht "`basename $n`" < $n
done | sort | gen_html

