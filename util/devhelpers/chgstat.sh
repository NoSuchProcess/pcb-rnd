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

if test -z $trunk
then
	trunk=../..
fi
dirs="$trunk/src $trunk/src_plugins"
import=2
exclude=""

echo "purging old blame files..." >&2
for d in $dirs
do
	b=`find $d -name '*.blm'`
	for n in $b
	do
		nb=${n%%.blm}
		if test ! -f $nb
		then
			echo " stale: $n"
			rm $n
		fi
	done
done

echo "Updating blame files..." >&2
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
					echo "blame: $f" >&2
					svn blame $f > $f.blm
				fi
				;;
		esac
	done
done

echo "Calculating stats..." >&2
for d in $dirs
do
	if test -d $d
	then
		cat `find $d -name '*.blm'`
	fi
done | awk -v import=$import -v "save=$1" '
		BEGIN {
			MASK[6735]++
			MASK[6720]++
			MASK[6719]++
			MASK[6718]++
			MASK[6713]++
			MASK[6710]++
			MASK[6709]++
			MASK[6707]++
			MASK[6705]++
			MASK[6703]++
			MASK[6701]++
			MASK[6700]++
			MASK[6699]++
			MASK[6690]++
			MASK[6689]++
			MASK[6686]++
			MASK[6683]++
			MASK[6681]++
			MASK[6679]++
			MASK[6668]++
			MASK[6667]++
			MASK[6657]++
			MASK[6610]++
			MASK[6581]++
			MASK[6570]++
			MASK[6963]++
###
			MASK[7353]++
			MASK[7344]++
			MASK[7333]++
			MASK[7312]++
			MASK[7300]++
			MASK[7297]++
			MASK[7295]++
			MASK[7293]++
			MASK[7292]++
			MASK[7291]++
			MASK[7289]++
			MASK[7288]++
			MASK[7287]++
			MASK[7217]++
			MASK[7213]++
			MASK[7212]++
			MASK[7211]++
			MASK[7185]++
			MASK[7117]++
			MASK[7086]++
			MASK[7056]++
			MASK[7044]++
			MASK[7042]++
			MASK[6975]++
			MASK[6966]++
			MASK[7357]++
##
			MASK[7371]++
			MASK[7370]++
			MASK[7369]++
			MASK[7367]++
			MASK[7365]++
			MASK[7372]++
##
			MASK[7560]++
			MASK[7564]++
			MASK[7595]++
			MASK[9342]++
			MASK[9597]++
			MASK[9766]++
			MASK[10659]++
			MASK[10662]++
			MASK[10727]++
			MASK[10752]++
			MASK[10801]++
			MASK[11028]++
			MASK[11034]++
			MASK[11035]++
			MASK[11036]++
			MASK[11074]++
			MASK[11301]++
			MASK[11302]++
			MASK[11304]++
			MASK[11320]++
			MASK[11321]++
			MASK[12102]++
			MASK[12101]++
			MASK[12100]++
			MASK[12099]++
			MASK[12098]++
			MASK[12097]++
			MASK[12096]++
			MASK[12093]++
			MASK[12092]++
			MASK[12091]++
			MASK[12089]++
			MASK[12088]++
			MASK[12087]++
			MASK[12086]++
			MASK[12083]++
			MASK[12081]++
			MASK[12080]++
			MASK[12078]++
			MASK[12068]++
			MASK[12067]++

			MASK[12120]++
			MASK[12121]++
			MASK[12122]++
			MASK[12123]++
			MASK[12124]++
			MASK[12125]++
			MASK[12126]++
			MASK[12228]++
			MASK[12273]++
			MASK[12280]++
			MASK[12292]++
			MASK[12293]++
			MASK[12377]++
			MASK[12420]++
			MASK[12421]++
			MASK[12485]++
			MASK[12563]++
			MASK[12618]++
			MASK[12640]++
			MASK[12678]++
			MASK[12695]++
			MASK[12700]++
			MASK[12721]++
			MASK[12757]++
			MASK[12788]++
			MASK[12824]++
			MASK[12954]++
			MASK[13008]++
			MASK[13516]++
			MASK[13558]++
			MASK[13890]++
			MASK[13910]++
			MASK[14168]++
			MASK[14174]++
			MASK[18570]++
			MASK[18604]++
			MASK[18647]++
			MASK[18650]++
			MASK[18652]++
			MASK[18719]++
			MASK[19685]++
			MASK[19686]++
			MASK[24965]++
		}

		function do_save()
		{
			print $0 > "Old.log"
		}

		{
			rev=int($1)
			if (((rev >= 3871) && (rev <= 3914)) || ((rev >= 4065) && (rev <= 4068)) || (rev == 4023) || (rev == 4033) || (rev == 4095) || (rev == 4096) || (rev == 4122)) {
# old plugins and export plugin import
				old++
				if (save)
					do_save();
			}
			else if ((rev == 4550) || ((rev <= 4548) && (rev >= 4536)) || ((rev <= 4534) && (rev >= 4530)) || ((rev <= 4528) && (rev >= 4524)) || ((rev <= 4522) && (rev >= 4502)) || ((rev <= 4500) && (rev >= 4493)) || ((rev <= 4491) && (rev >= 4486)) || ((rev <= 4633) && (rev >= 4562)) || (rev == 4776) || (rev == 4847) || (rev == 4850) || (rev == 4856) || (rev == 4863) || (rev == 4866) || (rev == 4878) || (rev == 4914) || (rev == 4916) || (rev == 5002) || (rev == 5014) || (rev == 5253) || (rev == 5487) || (rev == 5665)) {
# unravel
				old++
				if (save)
					do_save();
			}
			else if ((rev == 6046) || (rev == 6063) || (rev == 6066) || (rev == 6072) || (rev == 6077) || (rev == 6079) || (rev == 6083) || (rev == 6084) || (rev == 6365) || (rev == 6447) || (rev == 6449) || (rev == 6487) || (rev == 6491) || ((rev >= 6507) && (rev <= 6511))  || (rev == 6548)) {
# gtk splitup
				old++
				if (save)
					do_save();
			}
			else if (rev in MASK) {
# random maskout
				old++
				if (save)
					do_save();
			}
			else if ((rev <= import) || (rev == 1022) || (rev == 3539) || (rev == 4187) || (rev == 6230) || (rev == 6292) || (rev == 6293) || (rev == 6297) || (rev == 6298) || (rev == 6299)) {
				old++
				if (save)
					do_save();
			}
			else
				new++
		}

		END {
			print "old: " old
			print "new: " new
			print new/(old+new) * 100
		}
	'
