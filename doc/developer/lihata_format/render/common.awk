function get_name(node, ty, level)
{
	if (node "/name" in DATA)
		nm = DATA[node "/name"]
	else
		nm = qstrip(NAME[node])
	if (ty != "")
		nm =  ty ":" nm
	while(level > 0) {
		nm = "&nbsp;" nm
		level--
	}
	return nm
}

function gen_sub(root, level,    v, n, N, node, dst_children, dupsav)
{
	if (!(root in NAME)) {
		print "Error: path not found: " root > "/dev/stderr"
		return
	}
	v = children(N, root "/children")
	for(n = 1; n <= v; n++) {
		node = N[n]
		if (TYPE[node] == "symlink") {
			# normal node symlink: generate a link
#			print "SY:" node " " DATA[node] "^^^" sy_is_recursive(node) > "/dev/stderr"
			if (NAME[node] ~ "@dup") {
				dupsav = dupd_prefix
				if (dupsav == "")
					dupd_prefix = "dup" ++uniq_node_name "_"
				tbl_entry(DATA[node], level, root)
				gen_sub(DATA[node], level+1)
				if (dupsav == "")
					dupd_prefix = ""
			}
			else
				tbl_entry_link(node, DATA[node], level, root)
		}
		else if ((node "/children") in NAME) {
			tbl_entry(node, level, root)
			if (TYPE[node "/children"] == "symlink") {
				dst_children = DATA[node "/children"]
				sub("/children$", "", dst_children)
				gen_sub(dst_children, level+1)
			}
			else {
				gen_sub(node, level+1)
			}
		}
		else if (TYPE[node] == "hash")
			tbl_entry(node, level, root)
		else
			print "Unhandled child (unknown type): " node > "/dev/stderr"
	}
}
