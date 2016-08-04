#!/bin/sh

# TODO: rewrite this in C for better portability

awk -v "docdir=$1" '
	BEGIN {
		level = -1
		q = "\""
		cm = ","
	}

	function doc_head(fn, path)
	{
		if (fn in DOCS)
			return
		DOCS[fn]++
		print "<html><body>" > fn
		print "<h1>pcb-rnd conf tree</h1>" > fn
		print "<h2>subtree: " path "</h2>" > fn
		print "<table border=1>" > fn
		print "<tr><th>node name <th> type <td> flags <td> description" > fn

	}

	function doc_foot(fn)
	{
		print "</table></body></html>" > fn
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

	/^[ \t]*[\/][\/]/ { next }

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

		desc = $0
		if (desc ~ "/[*]") {
			sub("^.*/[*]", "", desc)
			sub("[*]/.*$", "", desc)
			sub("^[ \t]*", "", desc)
			sub("[ \t]*$", "", desc)
		}
		else
			desc = ""
		if (desc == "")
			desc = "<" name ">"

		flags = "";
		while(match(desc, "@[a-zA-Z_]+")) {
			flag=substr(desc, RSTART, RLENGTH)
			sub("[ \t]*" flag "[ \t]*", " ", desc)
			sub("^@", "",flag)
			flag="CFF_" toupper(flag)
			if (flags == "")
				flags = flag
			else
				flags = flags " | " flag
		}
		if (flags == "")
			flags = 0;
		sub("^[ \t]*", "", desc)
		sub("[ \t]*$", "", desc)

		path_tmp=path
		sub("^/", "", path_tmp)
		printf("conf_reg(%-36s %s %-16s %-25s %-25s %s, %s)\n", id cm, (array ? "array, " : "scalar,"), type cm, q path_tmp q cm, q name q cm, q desc q, flags)
		if (docdir != "") {
			path_tmp2 = path_tmp
			gsub("/", "_", path_tmp2)
			fn = docdir "/" path_tmp2 ".html"
			doc_head(fn, path_tmp)
			type2 = tolower(type)
			sub("^cfn_", "", type2)

			print "<tr><td>", name, "<td><a href=\"" type ".html\">", type2, "</a><td>", flags, "<td>", desc > fn
		}
	}

	/[}]/ {
		level--
		sub("[/][^/]*$", "", path)
	}


	END {
		if (level != -1)
			print "Error: unbalanced braces" > "/dev/stderr"
		for(fn in DOCS)
			doc_foot(fn)
	}
'
