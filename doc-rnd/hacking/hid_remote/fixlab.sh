#!/bin/sh
awk '
($2 == "->") {

	if (NF > 3) {
		attr=$0
		if (!(attr ~ "[[]"))
			attr = attr "[]"
	}
	else
		attr = "[]"

	tmp = "lbl" (++lbl)
	if (attr ~ "label")
		extra_label=""
	else
		extra_label="label=\"\""

	tmp_attr = attr
	attr1=attr
	attr2=attr

	if (extra_label == "")
		sub("^[^[]*[[]", "[style=filled fillcolor=\"#fefefe\" shape=plaintext ", tmp_attr)
	else {
#		tmp_attr = "[fixedsize=true width=0 shape=plaintext " extra_label "]"
		print $0
		next
	}

	sub("^[^[]*[[]", "[", attr1)
	sub("^[^[]*[[]", "[", attr2)
	sub("label=\".*\"", "", attr1)
	sub("label=\".*\"", "", attr2)
	sub("[[]", "[arrowhead=none ", attr1)

	print tmp, tmp_attr
	print $1, "->", tmp, attr1
	print tmp, "->", $3, attr2
	next
}
{ print $0 }
'