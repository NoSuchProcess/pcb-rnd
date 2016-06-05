#!/bin/sh
#   inc_weed_out - list #includes that can be removed without introducing warnings
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

# include weed-out: attempt to compile an object with each #include lines
# commented out one by one; if commenting an #include doesn't change compiler
# warnings, that include is potentially unneeded.

# Usage:
#  1. compile the whole project so that all dependencies of the target file are compiled
#  2. inc_weed_out.sh foo.c
#  3. check the output, remove #includes
#  4. compile by hand to check
#
#  NOTE: the script runs 'make foo.o' - your local Makefile needs to be able
#        to compile foo.o
#  NOTE: sometimes disabling multiple #includes have different effect than
#        disabling them one by one - always check manually.

# set up file names
fn_c=$1
fn_o=${1%.c}.o
fn_c_backup=$fn_c.inc_weed_out
fn_refo=$1.refo
fn_tmpo=$1.tmpo

# comment one #include (index is $1, save output in $2, print the #include line to stdout)
weed_out()
{
	awk -v "target=$1" -v "outf=$2" '
		/^#[ \t]*include/ {
			count++
			if (count == target) {
				print "/*" $0 "*/" > outf
				print $0
				found=1
				next
			}
		}
		{ print $0 > outf }
		END {
			exit(found!=1)
		}
	'
}

# make a backup
cp $fn_c $fn_c_backup

# generate initial/reference warning text
touch $fn_c
make $fn_o >/dev/null 2>$fn_refo

# loop over #include indices
cnt=1
while true
do
	# comment out the next #include or break if there's no more
	name=`weed_out $cnt < $fn_c_backup $fn_c`
	if test -z "$name"
	then
		break
	fi

	# test compile to see if we got new warnings compared to the reference
	make $fn_o 2>$fn_tmpo >/dev/null
	diff $fn_refo $fn_tmpo >/dev/null && echo REMOVE: $cnt $name

	# start over
	cnt=$(($cnt+1))
done

# clean up
mv $fn_c_backup $fn_c
rm $fn_refo $fn_tmpo
