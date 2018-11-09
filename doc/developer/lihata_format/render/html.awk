BEGIN {
	print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"
	print "<html>"
	print "<head>"
	print "<title> pcb-rnd developer manual - lihata file formats</title>"
	print "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=us-ascii\">"
	print "</head>"
	print "<body>"
}

function tbl_hdr(node, level)
{
	print "<tr><th align=left> type:name <th align=left> value <th align=left> <a href=\"#ver\">ver</a> <th align=left> description"
}

function get_valtype(node,    vt)
{
	vt = DATA[node "/valtype"]
	if (vt != "")
		vt = "<a href=" q "#valtype:" vt q ">" vt "</a>"
	else
		vt = "&nbsp;"
	return vt
}

function tbl_entry(node, level, parent     ,nm,vt,dsc,ty,vr)
{
	if (!(node in NAME)) {
		print "Error: path not found: " node > "/dev/stderr"
		return
	}
	ty = DATA[node "/type"]
	nm = get_name(node, ty, level)
	vt = get_valtype(node)
	vr = DATA[node "/ver"]
	if (vr == "") vr = "&nbsp;"
	dsc = qstrip(DATA[node "/desc"])
	print "<tr id=" q node q "><td> " nm " <td> " vt " <td> " vr " <td> " dsc
}

function tbl_entry_link(node, dst, level, parent     ,nm,vt,dsc,ty,vr)
{
	if (!(node in NAME)) {
		print "Error: path not found: " node > "/dev/stderr"
		return
	}
	if (!(dst in NAME)) {
		print "Error: path not found: " dst > "/dev/stderr"
		return
	}

	ty = DATA[dst "/type"]
	nm = get_name(node, ty, level)
	vt = get_valtype(dst)
	vr = DATA[dst "/ver"]
	if (vr == "") vr = "&nbsp;"
	dsc = qstrip(DATA[dst "/desc"])
	print "<tr id=" q node q "><td> " nm " <td> " vt " <td> " vr " <td> <a href=" q sy_href(dst) q ">" dsc " -&gt; </a>"
}

function gen_main(path,    v, n, N)
{
	print "<h3 id=" q path q ">"  DATA[path "/type"] ":" NAME[path] "</h3>"
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
	for(n = 1; n <= v; n++) {
		if (N[n] "/hide" in NAME)
			continue
		gen_main(N[n])
	}
}

function gen_types(path,    v, n, N, node)
{
	print "<table border=1 width=700>"
	print "<tr><th> type <th> description"

	v = children(N, path)
	for(n = 1; n <= v; n++) {
		node = N[n]
		print "<tr id=" q "valtype:" NAME[node] q ">"
		print "	<td>" NAME[node]
		print "	<td>" qstripnl(DATA[node])
	}
	print "</table>"
}


END {
	print "<h2> File format root nodes </h2>"
	print "<p>Each table below describes the full tree of one of the pcb-rnd file formats, from the root."
	gen_roots("/lht_tree_doc/roots")

	print "<h2> Common subtrees </h2>"
	print "<p>Each table below describes a subtree that usually does not specify a whole tree (thus they are usually not a valid file on their own). These subtrees are described in a separate table because they are used from multiple other trees."
	gen_roots("/lht_tree_doc/comm")

	print "<h2 id=\"types\"> Types </h2>"
	print "<p>"
	gen_types("/lht_tree_doc/types")

	print "<h2> Comments </h2>"
	print "<p id=\"ver\">ver column: <a href=\"../io_lihata_ver.html\">Format version</a> range the subtree may appear in."
	print "</body>"
	print "</html>"
}
