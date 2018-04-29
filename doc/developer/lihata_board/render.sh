#!/bin/sh

for n in *.lht
do
	lhtflat < $n
done | tee Flat | awk -F "[\t]" '
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

function sy_is_recursive(path,  dp)
{
	dp = DATA[path]
	gsub("/[0-9]::", "/", path)
	if (path ~ dp)
		rturn 1
	return 0
}

function sy_href(path)
{
	return "#" path
}

(($1 == "open") || ($1 == "data")) {
	TYPE[$3] = $2
	p = parent($3)
	if (CHILDREN[p] == "")
		CHILDREN[p] = $3
	else
		CHILDREN[p] = CHILDREN[p] "|" $3
	data=$4
	gsub("\\\\057", "/", data)
	DATA[$3] = data
	
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

function tbl_entry_link(node, dst, level     ,nm,vt,dsc,ty,vr)
{
	ty = DATA[dst "/type"]
	if (ty == "")
		nm = qstrip(NAME[node])
	else
		nm =  ty ":" qstrip(NAME[node])
	while(level > 0) {
		nm = "&nbsp;" nm
		level--
	}
	vt = DATA[dst "/valtype"]
	if (vt == "") vt = "&nbsp;"
	vr = DATA[dst "/ver"]
	if (vr == "") vr = "&nbsp;"
	dsc = qstrip(DATA[dst "/desc"])
	print "<tr><td> " nm " <td> " vt " <td> " vr " <td> <a href=" q sy_href(dst) q ">" dsc " -&gt; </a>"
}

function gen_sub(root, level,    v, n, N, node)
{
	v = children(N, root "/children")
	for(n = 1; n <= v; n++) {
		node = N[n]
		tbl_entry(node, level)
		if (TYPE[node] == "symlink") {
			# normal node symlink: generate a link
			print "SY:" node " " DATA[node] "^^^" sy_is_recursive(node) > "/dev/stderr"
			tbl_entry_link(node, DATA[node], level)
		}
		else if ((node "/children") in NAME) {
			print "-> recurse " node
			gen_sub(node, level+1)
		}
	}
}

function gen_main(path,    v, n, N)
{
	print "<h1 id=" q path q ">"  DATA[path "/type"] ":" NAME[path] "</h1>"
#	print "<p>"
#	print qstrip(DATA[path "/desc"])
	print "<p>"
	print "<table border=0 cellspacing=0>"
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
	gen_roots("/lht_tree_doc/comm")
}

'

