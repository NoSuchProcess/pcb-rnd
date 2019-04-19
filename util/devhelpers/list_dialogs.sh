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
		title=name
		if (name ~ "^\"") {
			sub("\"", "", name)
			sub("\".*", "", name)
		}
		else
			name = "<dyn>"

		sub("[^,]*,[^,]*, *", "", title)
		if (title ~ "^\"") {
			sub("\"", "", title)
			sub("\".*", "", title)
		}
		else
			title = "<dyn>"

		print name "\t" title "\t" file
	}
'
