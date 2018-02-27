#!/bin/sh
proot=../../../src_plugins

meta_deps="core io-standard io-alien hid-gtk2-dl hid-gtk2-gdk export export-sim export-extra auto extra cloud"

(echo '
<html>
<body>
'

for n in $proot/*/*.pup
do
	pkg=`basename $n`
	sed "s/^/$pkg /" < $n
done | awk -v "meta_deps=$meta_deps" '
	BEGIN {
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
		pkg = $1;
		sub("[.]pup$", "", pkg)
		if (pkg == "(core)") pkg="core"
		

		val=$3
		if (val == "(core)") val="core"
		cfg = pkg
		pkg = "pcb-rnd-" pkg
		val = "pcb-rnd-" val
	}

	($2 == "$package") {
		PKG[val] = PKG[val] " " cfg;
		PLUGIN[pkg] = val;
		IFILES[val] = IFILES[val] " " cfg ".pup " cfg ".so"
		if (val == "pcb-rnd-core") CFG_BUILDIN[cfg]++
		else CFG_PLUGIN[cfg]++
	}
	($2 == "dep") { PLUGIN_DEP[pkg] = PLUGIN_DEP[pkg] " " val }

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
					depson = "!!!" A[n]
				add_dep(pkg, depson)
			}
		}

		PKG_DEP["core"] = ""
		PKG_DEP["pcb-rnd"] = meta_deps
		PKG["pcb-rnd"] = "&lt;metapackage&gt;"

		print "<h3> Package summary and dependencies </h3>"
		print "<table border=1>"
		print "<tr><th> package <th> depends on (packages) <th> consists of (plugins)"

		for(pkg in PKG)
			print "<tr><th>" pkg "<td>" PKG_DEP[pkg] "<td>" PKG[pkg]
		print "</table>"

		print "<h3> Package description and files </h3>"
		print "<table border=1>"
		print "<tr><th> package <th> files <th> short <th> long"
		for(pkg in PKG) {
			if (SHORT[pkg] == "") SHORT[pkg] = "&nbsp;"
			if (LONG[pkg] == "") LONG[pkg] = "&nbsp;"
			print "<tr><th>" pkg "<td>" IFILES[pkg] "<td>" SHORT[pkg]  "<td>" LONG[pkg]
		}
		print "</table>"

		print "<h3> ./configure arguments </h3>"
		print "--all=disable"
		for(p in CFG_BUILDIN)
				print "--buildin-" p
		for(p in CFG_PLUGIN)
				print "--plugin-" p
	}
'

echo '
</body>
</html>
') > packages.html

