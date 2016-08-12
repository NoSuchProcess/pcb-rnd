#!/bin/sh
#   chgstat - svn statistics on lines changed since the fork
#   Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

trunk=../..
dirs="$trunk/src $trunk/src_plugins"
import=2
exclude=""


for d in $dirs
do
	for f in `find $d -name '*.[chly]'`
	do
		echo "blame: $f"
		case "$f" in
			*parse_y.c|*parse_y.h|*parse_l.c|*parse_l.h) ;;
#			*) svn blame $f > $f.blm;;
		esac
	done
done

for d in $dirs
do
	cat `find $d -name '*.blm'` 
done| awk -v import=$import '
		{
			rev=int($1)
			if ((rev <= import) || (rev == 1022))
				old++
			else
				new++
		}
		END {
			print "old: " old
			print "new: " new
			print new/(old+new) * 100
		}
	'
