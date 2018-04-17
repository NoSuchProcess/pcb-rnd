#!/bin/sh

lhtflat < tree.lht | awk -F "[\t]" '
BEGIN {
	q="\""
}

function parent(path)
{
	sub("/[^/]*$", "", path)
	return path
}

function children(DST, path)
{
	return split(CHILDREN[path], DST, "[|]")
}

(($1 == "open") || ($1 == "data")) {
	TYPE[$3] = $2
	p = parent($3)
	if (CHILDREN[p] == "")
		CHILDREN[p] = $3
	else
		CHILDREN[p] = CHILDREN[p] "|" $3
	DATA[$3] = $4
	name=$3
	sub("^.*/", "", name)
	sub(".*::", "", name)
	NAME[$3] = name
}

function qstrip(s)
{
	gsub("[\\\\]+164", " ", s)
	gsub("[\\\\]n", " ", s)
	return s
}

function tbl_hdr(node, level)
{
	print "<tr><th align=left> type:name <th align=left> value <th align=left> ver <th align=left> description"
}

function tbl_entry(node, level     ,nm,vt,dsc,ty,vr)
{
	ty = DATA[node "/type"]
	if (ty == "")
		nm = qstrip(NAME[node])
	else
		nm =  ty ":" qstrip(NAME[node])
	while(level > 0) {
		nm = "&nbsp;" nm
		level--
	}
	vt = DATA[node "/valtype"]
	if (vt == "") vt = "&nbsp;"
	vr = DATA[node "/ver"]
	if (vr == "") vr = "&nbsp;"
	dsc = qstrip(DATA[node "/desc"])
	print "<tr><td> " nm " <td> " vt " <td> " vr " <td> " dsc
}

function gen_sub(root, level,    v, n, N, node)
{
	v = children(N, root "/children")
	for(n = 1; n <= v; n++) {
		node = N[n]
		tbl_entry(node, level)
		if ((node "/children") in NAME) {
			print "-> recurse " node
			gen_sub(node, level+1)
		}
	}
}

function gen_main(path,    v, n, N)
{
	
	print "<h1 id=" q NAME[path] q ">"  DATA[path "/type"] ":" NAME[path] "</h1>"
#	print "<p>"
#	print qstrip(DATA[path "/desc"])
	print "<p>"
	print "<table border=0>"
	tbl_hdr()
	tbl_entry(path, 0)
	gen_sub(path, 1)
	print "</table>"
}

function gen_roots(rpath,    v, n, N)
{
	v = children(N, rpath)
	for(n = 1; n <= v; n++)
		gen_main(N[n])
}


END {
	gen_roots("/lht_tree_doc/roots")
}

'

