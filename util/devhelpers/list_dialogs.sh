#!/bin/sh

# assumes running from the source tree

trunk=$(dirname $0)/../..

grep PCB_DAD_NEW $trunk/src/*.c $trunk/src_plugins/*/*.c | awk '
	($1 ~ "TEMPLATE") { next }
	{
		file=$1
		name=$0
		sub(":$", "", file)
		sub(".*src/", "src/", file)
		sub(".*src_plugins/", "src_plugins/", file)
		sub(".*PCB_DAD_NEW[^(]*[(]", "", name)
		if (name ~ "^\"") {
			sub("\"", "", name)
			sub("\".*", "", name)
		}
		else
			name = "<dyn>"

		print name, file
	}
'
