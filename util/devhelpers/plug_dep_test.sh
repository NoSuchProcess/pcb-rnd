#!/bin/sh
#   plug_dep_test - detect missing plugin-plugin dependencies
#   Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#

# tip: installing ccache will speed this up a lot!

tmp="/tmp/pcb-rnd-pdt"
URL="svn://repo.hu/pcb-rnd/trunk"
conf="--debug"
make_paral="-j8"
#make_silent="-s"


# compile pcb-rnd, appending the compilation details to the log
# $1 is why we are compiling
compile()
{
	make $make_paral $make_silent >> PDT.$1.log 2>&1
}

# create a new checkout or update an existing checkout
checkout()
{
	mkdir -p "$tmp"
	cd "$tmp"
	svn checkout "$URL"
	cd trunk
}

# clean up the checkout directory, just in case it is an existing checkout
cleanup_checkout()
{
	make clean
	make distclean
	rm scconfig/config.cache.golden
}

# reference compilation: core only, no plugin enabled
ref_compile()
{
# create the golden scconig cache that will speed up further compilation
	./configure $conf --all=disable 2>&1 | tee PDT._ref.log
	grep -v "^/local/\|^/tmpasm/" scconfig/config.cache > scconfig/config.cache.golden

# reference compilation
	compile _ref
}


# print all plugin names (newline separated list)
list_plugs()
{
awk '
/^[^#]/ {
	plg=$0
	sub("/Plug.tmpasm.*", "", plg)
	sub(".*/", "", plg)
	print plg
}
' < src_plugins/plugins_ALL.tmpasm
}

# configure with only one plugin builtin and everything else disabled
# if there is compilation error, it's likely a broken dependency (linker
# error on another plugin that is now disabled)
test_plug_()
{
	make clean > PDT.$1.log 2>&1 && \
	./configure $conf --import=config.cache.golden --all=disable --buildin-$1 >> PDT.$1.log  2>&1 && \
	compile $1
}

# test one plugin and print the result
test_plug()
{
	echo -n "$1: "
	test_plug_ $1
	if test $? = 0
	then
		echo "ok"
	else
		echo "BROKEN, see PDT.$1.log"
	fi
}

test_all_plugins()
{
	cd "$tmp/trunk"
	plugs=`list_plugs`
	for plug in $plugs
	do
		test_plug $plug
	done
}

### main ###

checkout
cleanup_checkout
ref_compile
test_all_plugins
