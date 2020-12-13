#!/bin/sh
ROOT=../../..
proot=$ROOT/src_plugins
rnd_proot=$ROOT/src_3rd/librnd-local/src/librnd/plugins

### generate description.txt (file formats) ###

. $ROOT/util/devhelpers/awk_on_formats.sh

awk_on_formats  '
{ print $0 }

function out(dir, type  ,n,v,A,tmp)
{
	v = split(FMTS[dir, type], A, " *<br> *")
	if (v < 1) return
	print "  -", dir, type ":"
	for(n = 1; n <= v; n++) {
		tmp = A[n]
		sub("^ *", "", tmp)
		print "    * " tmp
	}
}

/(lihata)/ {
	t = split(types, T, " ")
	for(n = 1; n <= t; n++) {
		out("import", T[n]);
		out("export", T[n]);
	}
	exit
}
' < description.txt > description2.txt && mv description2.txt description.txt

### generate packages.html and auto/ ###

meta_deps="core io-standard io-alien hid-gtk2-gl hid-gtk2-gdk export export-sim export-extra auto extra cloud doc import-net"

(echo '
<html>
<body>
'

(
for n in $proot/*/*.pup
do
	pkg=`basename $n`
	sed "s/^/$pkg /" < $n
done
for n in $rnd_proot/*/*.pup
do
	pkg=`basename $n`
	sed "s/^/!$pkg /" < $n
done
for n in $proot/*/*.tmpasm
do
	sed "s@^@$n @" < $n
done
for n in $rnd_proot/*/*.tmpasm
do
	sed "s@^@!$n @" < $n
done
cat extra.digest
) | awk -v "meta_deps=$meta_deps"  '
	BEGIN {
		gsub(" ", " pcb-rnd-", meta_deps)
		sub("^", "pcb-rnd-", meta_deps)
		while((getline < "desc") == 1) {
			if ($0 ~ "^@") {
				pkg=$0
				sub("^@", "", pkg)
				getline SHORT[pkg] < "desc"
				continue
			}
			LONG[pkg] = LONG[pkg] $0 " "
		}
	}

	{
		if ($1 ~ "^[!]") {
			in_librnd = 1
			sub("^[!]", "", $1)
		}
		else
			in_librnd = 0
	}

	($1 ~ "@files") {
		pkg=$2
		files=$0
		sub("@files[ \t]*[^ \t]*[ \t]", "", files)
		IFILES[pkg] = files
		PKG[pkg] = "<i>n/a</i>"
	}

	($1 ~ "@appendfiles") {
		pkg=$2
		files=$0
		sub("@appendfiles[ \t]*[^ \t]*[ \t]", "", files)
		IFILES[pkg] = IFILES[pkg] " " files
	}

	($1 ~ "@appenddeps") {
		pkg=$2
		deps=$0
		sub("@appenddeps[ \t]*[^ \t]*[ \t]", "", deps)
		PKG_DEP[pkg] = PKG_DEP[pkg] " " deps
	}

	($1 ~ "[.]pup$") {
		pkg = $1;
		sub("[.]pup$", "", pkg)
		if (pkg == "(core)") pkg="core"
	}

	($1 ~ "[.]tmpasm$") {
		pkg = $1;
		sub("/Plug.tmpasm$", "", pkg)
		sub(".*/", "", pkg)
		if (pkg == "(core)") pkg="core"
	}

	($1 ~ "[.]pup$") {
		val=$3
		if (val == "(core)") val="core"
		cfg = pkg
		val = "pcb-rnd-" val
	}

	{
		pkg = "pcb-rnd-" pkg
	}

	($1 ~ "[.]pup$") && ($2 == "$package") {
		PKG[val] = PKG[val] " " cfg;
		PLUGIN[pkg] = val;
		if (val == "pcb-rnd-core") {
			CFG_BUILDIN[cfg]++
		}
		else {
			CFG_PLUGIN[cfg]++
print in_librnd, $1 > "L1"
			if (in_librnd)
				dir = "$LP"
			else
				dir="$P"
			IFILES[val] = IFILES[val] " " dir "/" cfg ".pup " dir "/" cfg ".so"
		}
	}

	($1 ~ "[.]pup$") && ($2 == "dep") { PLUGIN_DEP[pkg] = PLUGIN_DEP[pkg] " " val }

	($1 ~ "[.]tmpasm$") && ($3 == "/local/pcb/mod/CONFFILE") {
		fn=$4
		sub("[{][ \t]*", "", fn)
		sub("[ \t]*[}]", "", fn)
		if (in_librnd)
			dir = "$LC"
		else
			dir="$C"
		if (CONFFILE[PLUGIN[pkg]] == "")
			CONFFILE[PLUGIN[pkg]] = dir "/" fn
		else
			CONFFILE[PLUGIN[pkg]] = CONFFILE[PLUGIN[pkg]] " " dir "/" fn
	}

	function add_dep(pkg, depson,    ds)
	{
		if (pkg != depson) {
			ds = pkg "::" depson
			if (!(ds in DEP_SEEN)) {
				DEP_SEEN[ds] = 1
				PKG_DEP[pkg] = PKG_DEP[pkg] " " depson
			}
		}
	}

	function strip(s) {
		sub("^[ \t]*", "", s)
		sub("[ \t]*$", "", s)
		return s
	}

	END {
	
		# everything depends on core
		for(pkg in PKG)
			add_dep(pkg, "pcb-rnd-core")

		# calculate dependencies
		for(plg in PLUGIN_DEP) {
			v = split(PLUGIN_DEP[plg], A, " ")
			pkg = PLUGIN[plg]
			if (pkg == "") continue
			for(n = 1; n <= v; n++) {
				if (A[n] == "") continue
				depson = PLUGIN[A[n]]
				if (depson == "")
					depson = A[n]
				add_dep(pkg, depson)
			}
		}

		PKG_DEP["core"] = ""
		PKG_DEP["doc"] = ""
		PKG_DEP["pcb-rnd"] = meta_deps
		PKG["pcb-rnd"] = "&lt;metapackage&gt;"
		PKG["pcb-rnd-doc"] = "&nbsp;"
		IFILES["pcb-rnd-doc"] = "/usr/share/doc/*"

		print "<h3> Package summary and dependencies </h3>"
		print "<table border=1>"
		print "<tr><th> package <th> depends on (packages) <th> consists of (plugins)"

		for(pkg in PKG) {
			if (pkg == "pcb-rnd-core")
				print "<tr><th>" pkg "<td>" PKG_DEP[pkg] "<td>(builtin: " PKG[pkg] ")"
			else
				print "<tr><th>" pkg "<td>" PKG_DEP[pkg] "<td>" PKG[pkg]
			print strip(PKG_DEP[pkg]) >  "auto/" pkg ".deps"
			print pkg > "auto/List"
		}
		print "</table>"

		print "<h3> Package description and files </h3>"
		print "<table border=1>"
		print "<tr><th> package <th> files <th> short <th> long"
		for(pkg in PKG) {
			if (SHORT[pkg] == "") SHORT[pkg] = "&nbsp;"
			if (LONG[pkg] == "") LONG[pkg] = "&nbsp;"
			print "<tr><th>" pkg "<td>" IFILES[pkg] " <i>" CONFFILE[pkg] "</i>" "<td>" SHORT[pkg]  "<td>" LONG[pkg]
			print strip(IFILES[pkg] " " CONFFILE[pkg]) > "auto/" pkg ".files"
			print strip(SHORT[pkg]) > "auto/" pkg ".short"
			print strip(LONG[pkg]) > "auto/" pkg ".long"
		}
		print "</table>"
		print "<p>File prefixes:<ul>"
		print "	<li> $P: pcb-rnd plugin install dir (e.g. /usr/lib/pcb-rnd/)"
		print "	<li> $C: pcb-rnd conf dir (e.g. /usr/share/pcb-rnd/)"
		print "	<li> $LP: librnd plugin install dir (e.g. /usr/lib/librnd/)"
		print "	<li> $LC: librnd conf dir (e.g. /usr/share/librnd/)"
		print "	<li> $PREFIX: installation prefix (e.g. /usr)"
		print "</ul>"

		print "<h3> ./configure arguments </h3>"
		print "--all=disable"
		print "--all=disable" > "auto/Configure.args"

		for(p in CFG_BUILDIN) {
			print "--buildin-" p
			print "--buildin-" p > "auto/Configure.args"
		}
		for(p in CFG_PLUGIN) {
			print "--plugin-" p
			print "--plugin-" p > "auto/Configure.args"
		}
	}
'

echo '
</body>
</html>
') > packages.html

