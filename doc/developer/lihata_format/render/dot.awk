BEGIN {
	nl = "\\n"
}

function tbl_entry(node, level, nparent     ,nm,vt,dsc,ty,vr, url, tip,duppar,grp,grp_parent)
{
	ty = DATA[node "/type"]
	nm = get_name(node, ty, 0)
	vt = DATA[node "/valtype"]
	vr = DATA[node "/ver"]
	grp_parent = parent(parent(node))
	grp = DATA[grp_parent "/dot_group"]
	dsc = qstrip(DATA[node "/desc"])
	gsub("\"", "\\\"", dsc)
	tip=" tooltip=" q dsc q
	url=" URL=" q "tree.html#" node q

	if (dupd_prefix != "") {
		node = dupd_prefix node
		DUPD[node] ++

		if (nparent != "") {
			duppar = dupd_prefix nparent
			if (duppar in DUPD)
				nparent = duppar
		}
	}

	print "	" q node q "	[label=" q nm nl vt nl vr q url tip "]" >fn
	if (nparent != "")
		print "	" q nparent q "	->	" q node q > fn
	if (grp != "") {
		if (LAST_GRP_SIBL[nparent] != "") {
			print "		" q LAST_GRP_SIBL[nparent] q "	->	" q node q  "[style=invis]" > fn
		}
		LAST_GRP_SIBL[nparent] = node
	}
}

function tbl_entry_link(node, dst, level, parent     ,nm,vt,dsc,ty,vr,contid,url,tip,dr)
{
	ty = DATA[node "/type"]
	nm = get_name(node, ty, 0)
	vt = DATA[node "/valtype"]
	vr = DATA[node "/ver"]
	dsc = qstrip(DATA[node "/desc"])
	gsub("\"", "\\\"", dsc)
	dr = dst
	sub("^.*/", "", dr)
	url=" URL=" q dr ".svg" q
	tip=" tooltip=" q dsc q


	if (dupd_prefix != "") {
		node = dupd_prefix node
		DUPD[node] ++

		if (parent != "") {
			duppar = dupd_prefix parent
			if (duppar in DUPD)
				parent = duppar
		}
	}


	print "	" q node q "	[label=" q nm " ->" nl vt nl vr nl q url tip " shape=plaintext]" >fn
	if (parent != "")
		print "	" q parent q "	->	" q node q > fn
}

function gen_graph(rpath,    v, n, N, name)
{
	name = get_name(rpath, DATA[rpath "/type"])
	fn=name
	sub(".*[:/]", "", fn)
	gsub("[*]", "", fn)
	fn = "../" fn ".dot"

	print "digraph " q name q " {" > fn
	tbl_entry(rpath, 0)
	gen_sub(rpath, 1)
	print "}" > fn
	close(fn)
}

function gen_graphs(rpath,    v, n, N, name)
{
	v = children(N, rpath)
	for(n = 1; n <= v; n++) {
		if (N[n] "/hide" in NAME)
			continue
		gen_graph(N[n])
	}
}

END {
	gen_graphs("/lht_tree_doc/roots")
	gen_graphs("/lht_tree_doc/comm")
}
