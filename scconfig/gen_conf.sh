#!/bin/sh

# TODO: rewrite this in C for better portability

awk '
	BEGIN {
		level = -1
		q = "\""
		cm = ","
	}
	/[{]/ { level++ }

	/struct.*/ {
		name = $0
		sub(".*struct[ \t]*", "", name)
		sub("[ \t{].*", "", name)
		if (level == 0)
			path = ""
		else
			path = path "/" name
	}

	/CFT_/ {
		if (level < 1)
			next
		name=$0
		sub("^.*CFT_", "CFN_", name)
		sub(";.*", "", name)
		type=name
		sub("[ \t]*[^ \t]*$", "", type)
		sub("[^ \t]*[ \t]*", "", name)
		array = (name ~ "[[]")
		if (array)
			sub("[[].*$", "", name)
		id = path
		sub("^/", "", id)
		gsub("/", ".", id)
		id = id "." name

		path_tmp=path
		sub("^/", "", path_tmp)
		printf("conf_reg(%-36s %s %-16s %-25s %s)\n", id cm, (array ? "array, " : "scalar,"), type cm, q path_tmp q cm, q name q)
		
	}

	/[}]/ {
		level--
		sub("[/][^/]*$", "", path)
	}


	END {
		if (level != -1)
			print "Error: unbalanced braces" > "/dev/stderr"
	}
'