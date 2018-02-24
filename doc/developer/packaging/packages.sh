#!/bin/sh
proot=../../../src_plugins

(echo '
<html>
<body>
<table border=1>
<tr><th> package <th> depends on (packages) <th> consists of (plugins)
'

for n in $proot/*/*.pup
do
	pkg=`basename $n`
	sed "s/^/$pkg /" < $n
done | awk '
	{ pkg = $1; sub("[.]pup$", "", pkg) }
	($2 == "$package") { PKG[$3] = PKG[$3] " " pkg; PLUGIN[pkg] = $3 }
	($2 == "dep") { PLUGIN_DEP[pkg] = PLUGIN_DEP[pkg] " " $3 }

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
		for(pkg in PKG)
			add_dep(pkg, "(core)")

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

		
		for(pkg in PKG) {
			print pkg "|" PKG_DEP[pkg] "|" PKG[pkg]
		}
	}
' | sort | awk -F "[|]" '
	{ print "<tr><th>" $1 "<td>" $2 "<td>" $3 }
'

echo '
</table>
</body>
</html>
') > packages.html

