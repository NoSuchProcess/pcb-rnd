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

c99tree awk \
  --gtx 'events=([] /+  d=[type == DECL]) && (d / i=[name ~ "GPMI_EVENT"]) && (i .+ [type == TYPE] . a=[type  == ARGLIST] ) && (d : [loc_is_local == "1"])' \
  --gtx 'funcs=([] /+  d=[type == DECL]) && (d / i=[!name ~ "GPMI_EVENT"]) && (i .+ [type == TYPE] . a=[type  == ARGLIST] ) && (d : [loc_is_local == "1"])' \
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

function get_pre_comment(TREE, uid   ,cmt_uid,cmt)
{
	cmt_uid = TREE[uid, C99F_TWIN_PARENT]
	while(cmt_uid != "") {
		cmt_uid = TREE[cmt_uid, C99F_PREV]
		if (TREE[cmt_uid, C99F_TYPE] == C99T_COMMENT) {
			cmt = TREE[cmt_uid, C99F_NAME]
			break
		}
	}
	sub("^[/][*]", "", cmt)
	sub("[*][/]$", "", cmt)
	return cmt
}

BEGIN {
	c99tree_unknown(TREE)
	gtx_init(TREE)

#	source=load("'$fn'")

	print "<h3> Events </h3>"
	events = gtx_find_results(TREE, "events")
	for(r = 0; 1; r++) {
		if (gtx_get_map(TREE, MAP, events, r) == "")
			break
		id = TREE[MAP["i"], C99F_NAME]
		sub("^GPMI_EVENT__", "", id)
		proto = to_c(TREE, MAP["d"])
		sub("^[ \t]*void[ \t]*GPMI_EVENT__", "", proto)
# proto = getsrc(source, MAP["d"])

		print "<H4> " proto "</H4>"
		print "<pre>"
		print get_pre_comment(TREE, MAP["a"])
		print "</pre>"
	}

	print "<h3> Functions </h3>"
	funcs = gtx_find_results(TREE, "funcs")
	for(r = 0; 1; r++) {
		if (gtx_get_map(TREE, MAP, funcs, r) == "")
			break
		id = TREE[MAP["i"], C99F_NAME]
		sub("^GPMI_EVENT__", "", id)
#		print "<i>" getsrc(source, MAP["d"]) "</i>"
		proto = to_c(TREE, MAP["d"])
# proto = getsrc(source, MAP["d"])

		print "<H4>", proto, "</H4>"
		print "<pre>"
		print get_pre_comment(TREE, MAP["a"])
		print "</pre>"
	}
}
'

