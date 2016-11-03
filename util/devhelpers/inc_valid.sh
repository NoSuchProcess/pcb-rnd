#!/bin/sh
#   inc_weed_out - check whether a source file has valid #includes for pcb-rnd
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

# include validation:
#  - in .c files config.h must be included
#  - warn for duplicate #includes

# Usage:
#  1. compile the whole project so that all dependencies of the target file are compiled
#  2. inc_valid.sh foo.c
#  3. check the output, fix #includes
#  4. compile by hand to check
#
#  NOTE: conditional code also requires manual examination 

# set up file names
fn_c=$1

# comment one #include (index is $1, save output in $2, print the #include line to stdout)
valid()
{
	awk -v "fn=$1" '
		/^#[ \t]*include/ {
			SEEN[$2]++
		}
		END {
			for(h in SEEN)
				if (SEEN[h] > 1)
					print "W " fn ": multiple include of " h
			if (! ("\"config.h\"" in SEEN))
				print "E " fn ": missing config.h"
		}
	' < "$1"
}

for n in "$@"
do
	valid "$n"
done
