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
		BEGIN {
			GTK[6735]++
			GTK[6720]++
			GTK[6719]++
			GTK[6718]++
			GTK[6713]++
			GTK[6710]++
			GTK[6709]++
			GTK[6707]++
			GTK[6705]++
			GTK[6703]++
			GTK[6701]++
			GTK[6700]++
			GTK[6699]++
			GTK[6690]++
			GTK[6689]++
			GTK[6686]++
			GTK[6683]++
			GTK[6681]++
			GTK[6679]++
			GTK[6668]++
			GTK[6667]++
			GTK[6657]++
			GTK[6610]++
			GTK[6581]++
			GTK[6570]++
			GTK[6963]++
###
			GTK[7353]++
			GTK[7344]++
			GTK[7333]++
			GTK[7312]++
			GTK[7300]++
			GTK[7297]++
			GTK[7295]++
			GTK[7293]++
			GTK[7292]++
			GTK[7291]++
			GTK[7289]++
			GTK[7288]++
			GTK[7287]++
			GTK[7217]++
			GTK[7213]++
			GTK[7212]++
			GTK[7211]++
			GTK[7185]++
			GTK[7117]++
			GTK[7086]++
			GTK[7056]++
			GTK[7044]++
			GTK[7042]++
			GTK[6975]++
			GTK[6966]++
			GTK[7357]++
##
			GTK[7371]++
			GTK[7370]++
			GTK[7369]++
			GTK[7367]++
			GTK[7365]++
			GTK[7372]++
##
			GTK[7560]++
			GTK[7564]++
			GTK[7595]++
		}

		{
			rev=int($1)
			if (((rev >= 3871) && (rev <= 3914)) || ((rev >= 4065) && (rev <= 4068)) || (rev == 4023) || (rev == 4033) || (rev == 4095) || (rev == 4096) || (rev == 4122)) {
# old plugins and export plugin import
				old++
			}
			else if ((rev == 4550) || ((rev <= 4548) && (rev >= 4536)) || ((rev <= 4534) && (rev >= 4530)) || ((rev <= 4528) && (rev >= 4524)) || ((rev <= 4522) && (rev >= 4502)) || ((rev <= 4500) && (rev >= 4493)) || ((rev <= 4491) && (rev >= 4486)) || ((rev <= 4633) && (rev >= 4562)) || (rev == 4776) || (rev == 4847) || (rev == 4850) || (rev == 4856) || (rev == 4863) || (rev == 4866) || (rev == 4878) || (rev == 4914) || (rev == 4916) || (rev == 5002) || (rev == 5014) || (rev == 5253) || (rev == 5487) || (rev == 5665)) {
# unravel
				old++
			}
			else if ((rev in GTK) || (rev == 6046) || (rev == 6063) || (rev == 6066) || (rev == 6072) || (rev == 6077) || (rev == 6079) || (rev == 6083) || (rev == 6084) || (rev == 6365) || (rev == 6447) || (rev == 6449) || (rev == 6487) || (rev == 6491) || ((rev >= 6507) && (rev <= 6511))  || (rev == 6548)) {
# gtk splitup
				old++
			}
			else if ((rev <= import) || (rev == 1022) || (rev == 3539) || (rev == 4187) || (rev == 6230) || (rev == 6292) || (rev == 6293) || (rev == 6297) || (rev == 6298) || (rev == 6299))
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
