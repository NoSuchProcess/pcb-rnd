#!/bin/sh

# TODO: rewrite this in C for better portability

if test -z "$AWK"
then
	AWK="awk"
fi

# Trickery: the name (for path) of the current struct is learned only at
# the end, when the struct is closed because struct type names would be
# global and pollute the namespace while variable names are local. So to
# pick up trailing variable names:
#  - each struct gets an integer uid, 0 being the root; structs remember
#    their ancestry using PARENT[uid].
#  - each entry is remembered in an SRC[uid, n], where n is between 0
#    and LEN[uid]
#  - the output is build in END{} after loading everything into those arrays


$AWK -v "docdir=$1" '
	BEGIN {
		level = -1
		q = "\""
		cm = ","
		uids = 0
		stderr = "/dev/stderr"
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

	function gen(path, src_line,          id, name, type, array, desc, flags,path_tmp,type2,fn)
	{
		name=src_line
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

		desc = src_line
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

	function push_uid()
	{
		level++
		UID_STACK[level] = uid
	}

	function pop_uid()
	{
		level--
		uid = UID_STACK[level]
	}

	function gen_path(uid     ,path) {
		path = ""
		while(LEVEL[uid] > 0) {
			path = NAME[uid] "/" path
			uid = PARENT[uid]
		}
		sub("/$", "", path)
		return path
	}

	/[{]/ {
		uid = uids++
		LEN[uid] = 0
		push_uid()
	}

	/^[ \t]*[\/][\/]/ { next }

	/CFT_/ { SRC[uid, LEN[uid]++] = $0 }

	/[}]/ {
		name = $0
		sub(".*}[ \t]*", "", name)
		sub("[ \t{].*", "", name)
		sub(";.*", "", name);

		PARENT[uid] = UID_STACK[level-1]
		NAME[uid] = name
		LEVEL[uid] = level

		pop_uid()
	}


	END {
		if (level != -1)
			print "Error: unbalanced braces" > "/dev/stderr"

		if (LEN[0] != 0) {
			for(n = 0; n < LEN[0]; n++)
				print "gen_conf ERROR: ignoring line \"" SRC[0, n] "\" in root - needs to be within a struct" > stderr
		}

		for(uid = 0; uid < uids; uid++) {
			path = gen_path(uid)
			for(n = 0; n < LEN[uid]; n++)
				gen(path, SRC[uid, n])
		}

		for(fn in DOCS)
			doc_foot(fn)
	}
'
