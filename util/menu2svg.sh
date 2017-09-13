#!/bin/sh
#   menu2svg - visualize a menu file using graphviz
#   Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

fn="$1"
if test -z $fn
then
	fn="../src/pcb-menu-gtk.lht"
fi

lhtflat < $fn  | tee LHT | awk -F "[\t]" '
!($3 ~ "^//main_menu/") { next }

{
	menu_path = $3
	sub("//main_menu/", "", menu_path)
	gsub("/submenu", "", menu_path)
	gsub("[0-9]+::", "", menu_path)
}

(($1 == "open") && ($2 == "hash")) {
	if (current != "")
		print current
	current = menu_path
}

(($1 == "data") && ($2 == "text") && (current != "")) {
	if ($3 ~ "/a$") {
		PROPS["key"] = $4
		sub("<[Kk]ey>", " ", PROPS["key"])
	}
	else if ($3 ~ "/action$") {
		PROPS["act"] = $4
	}


}

(($1 == "close") && (current != "")) {
	print current "\t" PROPS["key"] "\t" PROPS["act"]
	current = ""
	delete PROPS
}
' | tee Menu.flat | awk -F "[\t]" '
BEGIN {
	q = "\""
	print "digraph menu {"
	print "rankdir=LR;"
#	print "ranksep=4;"
	print "node [shape=record];"

	reg_node("/")
}

function reg_node(path)
{
	last_id++
	PATH2ID[path] = last_id
	ID2PATH[last_id] = path
}

function add_ch(parent, child)
{
	if (NUM_CH[parent] == "")
		NUM_CH[parent] = 0
	if (IS_CH[parent, child])
		return
	IS_CH[parent, child] = 1
	CH[parent, NUM_CH[parent]] = child

	NUM_CH[parent]++
}

{
	path=$1
	KEY[path] = $2
	ACT[path] = $3
	reg_node(path)
	while(path ~ "/") {
		parent = path
		sub("/[^/]*$", "", parent)
		add_ch(parent, path)
		path = parent
	}
	add_ch("/", path)
}

function gen_menus(parent   ,n,child,short,chp)
{
	short=parent
	sub(".*/", "", short)
	if (!(parent in PATH2ID))
		reg_node(parent)
	printf "	m" PATH2ID[parent] " [label=\"<menu> [[" short "]]"
	for(n = 0; n < NUM_CH[parent]; n++) {
		child=CH[parent, n]
		short=child
		sub(".*/", "", short)
		printf("|<m%d> %s", PATH2ID[child], short)
	}
	print "\"]"

	for(n = 0; n < NUM_CH[parent]; n++) {
		child=CH[parent, n]
		if (NUM_CH[child] > 0) {
			chp = gen_menus(child)
			print "	m" PATH2ID[parent] ":m" PATH2ID[child] " -> " chp ":menu"
		}
	}

	return "m" PATH2ID[parent]
}


END {
	for(n = 0; n < NUM_CH["/"]; n++) {
		child=CH["/", n]
		curr = gen_menus(child)
		if (last != NULL) {
			print "tmp" n " [shape=point]"
			print last ":menu -> tmp" n " [weight=10000 arrowhead=none]"
			print "tmp" n " -> " curr ":menu [weight=10000 arrowhead=none]"
		}
		last = curr
	}
	print "}"
}
' > Menu.dot

dot -Tsvg Menu.dot > Menu.svg

