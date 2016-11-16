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

echo "Updating blame files..."
for d in $dirs
do
	for f in `find $d -name '*.[chly]'`
	do
		case "$f" in
			*parse_y.c|*parse_y.h|*parse_l.c|*parse_l.h|*_sphash*) ;;
			*)
				src_date=`stat -c %Y $f`
				if test -f $f.blm
				then
					blm_date=`stat -c %Y $f.blm`
				else
					blm_date=0
				fi
				if test $src_date -gt $blm_date
				then
					echo "blame: $f"
					svn blame $f > $f.blm
				fi
				;;
		esac
	done
done

echo "Calculating stats..."
for d in $dirs
do
	cat `find $d -name '*.blm'` 
done| awk -v import=$import '
		{
			rev=int($1)
			if (((rev >= 3871) && (rev <= 3914)) || ((rev >= 4065) && (rev <= 4068)) || (rev == 4023) || (rev == 4033) || (rev == 4095) || (rev == 4096) || (rev == 4122)) {
# old plugins and export plugin import
				old++
			}
			else if ((rev == 4550) || ((rev <= 4548) && (rev >= 4536)) || ((rev <= 4534) && (rev >= 4530)) || ((rev <= 4528) && (rev >= 4524)) || ((rev <= 4522) && (rev >= 4502)) || ((rev <= 4500) && (rev >= 4493)) || ((rev <= 4491) && (rev >= 4486)) || ((rev <= 4633) && (rev >= 4562)) || (rev == 4776) || (rev == 4847) || (rev == 4850) || (rev == 4856) || (rev == 4863) || (rev == 4866) || (rev == 4878) || (rev == 4914) || (rev == 4916) || (rev == 5002) || (rev == 5014)) {
# unravel
				old++
			}
			else if ((rev <= import) || (rev == 1022) || (rev == 3539) || (rev == 4187))
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
