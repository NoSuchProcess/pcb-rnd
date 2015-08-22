#!/bin/sh

# generate reference html from gpmi headers
# requires c99tree (probably svn HEAD version)

prinit='
#define GPMI_GPMI_PACKAGE_H
#define multiple __attribute__((multiple))
#define dynamic __attribute__((dynamic))
#define direct __attribute__((direct))
#define nowrap __attribute__((nowrap))

#define gpmi_define_event(name) void GPMI_EVENT__ ## name
'

fn=$1
root=$2
shift 2

pkgfn=`basename $fn`
pkg=${pkgfn%%.h}
echo "<html>
<body>
<h1>PCB GPMI</h1>
<h2>Reference manual for package $pkg</h2>
<small>Automatically generated from $pkgfn</small>
"


c99tree awk -I$root "$@" \
  --gtx 'events=([] /+  d=[type == DECL]) && (d / i=[name ~ "GPMI_EVENT"]) && (i .+ [type == TYPE] . a=[type  == ARGLIST] ) && (d : [loc_is_local == "1"])' \
  --gtx 'funcs=([] /+  d=[type == DECL]) && (d / i=[!name ~ "GPMI_EVENT"]) && (i .+ [type == TYPE] . [] >* a=[type  == ARGLIST] ) && (d : [loc_is_local == "1"])' \
  --gtx 'enums=([] /+  i=[type == ENUM]) && (i : [loc_is_local == "1"])' \
  --paste "$prinit" $fn --awk-s '

function load(fn,    s)
{
	while((getline line < fn) > 0)
		s = s line "\n"
	return s
}

#function getsrc(source, uid   ,L)
#{
#	print TREE[uid, C99F_LOCSTR]
#	if (split(TREE[uid, C99F_LOCSTR], L, ":") != 2)
#		return "???"
#
#	print L[1] "-" L[2] 
#	return substr(source, L[1], L[2]-L[1])
#}

function print_tree(TREE, uid, ind,    jump_next,s)
{
	while(uid != "") {
#print "U", ind, uid, "ty=" C99SYM[TREE[uid, C99F_TYPE]] " nm=" TREE[uid, C99F_NAME]
		s = s ind C99SYM[TREE[uid, C99F_TYPE]] " " TREE[uid, C99F_NAME] " [0:0] (-1)\n"
		s = s print_tree(TREE, TREE[uid, C99F_CHILD, 0], ind " ", 1)
		if (!jump_next)
			break;
		uid = TREE[uid, C99F_NEXT]
	}
	return s
}

function to_c(TREE, uid   ,tr,line,all)
{
	tr = "FILE  (%l)\n GRAMMAR_TREE  [0:0] (-1)\n"
	tr = tr print_tree(TREE, uid, "  ")

	cmd = "echo \"" tr "\" | c99tree-cc -tind -Tc"
	close(cmd)
	while((cmd | getline line) > 0)
		all = all line " "
	close(cmd)
	return all
}

function get_any_comment(TREE, uid, bump_uid, dir   ,cmt_uid,cmt)
{
	cmt_uid = TREE[uid, C99F_TWIN_PARENT]
	bump_uid = TREE[bump_uid, C99F_TWIN_PARENT]
	while(cmt_uid != "") {
		cmt_uid = TREE[cmt_uid, dir]
		if (cmt_uid == bump_uid) {
			cmt = cmt "&lt;comment missing in the header&gt;"
			break
		}
		if (TREE[cmt_uid, C99F_TYPE] == C99T_COMMENT) {
			cmt = TREE[cmt_uid, C99F_NAME]
			break
		}
	}
	sub("^[/][*]", "", cmt)
	sub("[*][/]$", "", cmt)
	return cmt
}

function get_pre_comment(TREE, uid, bump_uid)
{
	return get_any_comment(TREE, uid, bump_uid, C99F_PREV)
}

function get_post_comment(TREE, uid, bump_uid)
{
	return get_any_comment(TREE, uid, bump_uid, C99F_NEXT)
}

BEGIN {
	pkg="'$pkg'"
	c99tree_unknown(TREE)
	gtx_init(TREE)

#	source=load("'$fn'")

	print "<h3> Enums </h3>"
	print "<dl>"
	print "<p>Enum values should be passed on as strings."
	enums = gtx_find_results(TREE, "enums")
	for(r = 0; 1; r++) {
		if (gtx_get_map(TREE, MAP, enums, r) == "")
			break
		id = TREE[MAP["i"], C99F_NAME]

		print "<a id=\"" id "\">"
		print "<H4> " id "</H4>"
		print "<pre>"
		print get_pre_comment(TREE, MAP["i"])
		print "</pre>"

		print "<table border=1>"
		print "<tr><th>value <th>meaning"
		for(c = 0; c < TREE[MAP["i"], C99F_CHILDREN]; c++) {
			uid = TREE[MAP["i"], C99F_CHILD, c]
			nuid = TREE[MAP["i"], C99F_CHILD, c+1]
			if (nuid == "")
				nuid=TREE[TREE[MAP["i"], C99F_PARENT], C99F_NEXT]
			print "<tr><td>", TREE[uid, C99F_NAME], "<td>", get_post_comment(TREE, uid, nuid)
		}
		print "</table>"
	}
	print "</dl>"


	print "<h3> Events </h3>"
	print "<dl>"
	print "<p>Events do not have return value. The first argument is always <a href=\"event_id.html\">the even id</a>"
	events = gtx_find_results(TREE, "events")
	for(r = 0; 1; r++) {
		if (gtx_get_map(TREE, MAP, events, r) == "")
			break
		id = TREE[MAP["i"], C99F_NAME]
		sub("^GPMI_EVENT__", "", id)
		proto = to_c(TREE, MAP["d"])
		sub("^[ \t]*void[ \t]*GPMI_EVENT__", "", proto)
		sub("[(]", "(int event_id,", proto)
# proto = getsrc(source, MAP["d"])

		print "<a id=\"" id "\">"
		print "<H4> " proto "</H4>"
		print "<pre>"
		luid=TREE[MAP["d"], C99F_PREV]
		print get_pre_comment(TREE, MAP["a"], luid)
		print "</pre>"
	}
	print "</dl>"

	print "<h3> Functions </h3>"
	print "<dl>"
	print "<p>The following functions are registered in script context."
	funcs = gtx_find_results(TREE, "funcs")
	for(r = 0; 1; r++) {
		if (gtx_get_map(TREE, MAP, funcs, r) == "")
			break
		id = TREE[MAP["i"], C99F_NAME]
		if (id ~ "^package_" pkg "_")
			continue
		if (id ~ "^pkg_" pkg "_")
			continue

		sub("^GPMI_EVENT__", "", id)
#		print "<i>" getsrc(source, MAP["d"]) "</i>"
		proto = to_c(TREE, MAP["d"])
# proto = getsrc(source, MAP["d"])

		gsub("[(][ \t]*", "(", proto)
		print "<a id=\"" id "\">"
		print "<H4>", proto, "</H4>"
		print "<pre>"
		luid=TREE[MAP["d"], C99F_PREV]
		print get_pre_comment(TREE, MAP["a"], luid)
		print "</pre>"
	}
	print "</dl>"
}
'

echo "
</body>
</html>
"


