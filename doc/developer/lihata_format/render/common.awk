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

function dup_begin(DUPSAV)
{
	DUPSAV[1] = dupd_prefix
	if (DUPSAV[1] == "")
		dupd_prefix = "dup" ++uniq_node_name "_"
}

function dup_end(DUPSAV)
{
	dupd_prefix = DUPSAV[1]
}

function gen_sub(root, level, parent,   v, n, N, node, dst_children, DUPSAV)
{
	if (!(root in NAME)) {
		print "Error: path not found: " root > "/dev/stderr"
		return
	}
	if (parent == "")
		parent = root
	v = children(N, root "/children")
	for(n = 1; n <= v; n++) {
		node = N[n]
		if (TYPE[node] == "symlink") {
			# normal node symlink: generate a link
#			print "SY:" node " " DATA[node] "^^^" sy_is_recursive(node) > "/dev/stderr"
			dup_begin(DUPSAV)
			if (NAME[node] ~ "@dup") {
				tbl_entry(DATA[node], level, parent)
				gen_sub(DATA[node], level+1)
			}
			else
				tbl_entry_link(node, DATA[node], level, parent)
			dup_end(DUPSAV)
		}
		else if ((node "/children") in NAME) {
			tbl_entry(node, level, parent)
			if (TYPE[node "/children"] == "symlink") {
				dup_begin(DUPSAV)
				dst_children = DATA[node "/children"]
				sub("/children$", "", dst_children)
				gen_sub(dst_children, level+1, node)
				dup_end(DUPSAV)
			}
			else {
				gen_sub(node, level+1)
			}
		}
		else if (TYPE[node] == "hash")
			tbl_entry(node, level, parent)
		else
			print "Unhandled child (unknown type): " node > "/dev/stderr"
	}
}
