#!/bin/sh
ROOT=../../..
proot=$ROOT/src_plugins

# Get librnd requirement from INSTALL so it doesn't need to be maintained
# multiple locations
librnd_min_ver()
{
	awk -v "which=$1" '
		/librnd >=/ {
			ver=$0
			sub("^.*>=", "", ver)
			sub("[ \t].*$", "", ver)
			split(ver, V, "[.]")
			if (which == "major")
				print V[1]
			else if (which == "minor")
				print V[2]
			else if (which == "patch")
				print V[3]
			else
				print ver
		}
	' < $ROOT/INSTALL
}


# major version of librnd
RNDV=`librnd_min_ver major`
RNDVER=`librnd_min_ver`

if test -f $ROOT/Makefile.conf
then
	LIBRND_ROOT=`make -f librnd_root.mk`
	LIBRND_LIBDIR=`make -f librnd_root.mk libdir`
fi

if test -z "$LIBRND_ROOT"
then
	if test -f /usr/local/share/librnd${RNDV}/librnd_packages.sh
	then
		LIBRND_ROOT=/usr/local
	else
		LIBRND_ROOT=/usr
	fi
	LIBRND_LIBDIR=$LIBRND_ROOT/lib/librnd${RNDV}
fi

if test -f $LIBRND_ROOT/share/librnd${RNDV}/librnd_packages.sh
then
	. $LIBRND_ROOT/share/librnd${RNDV}/librnd_packages.sh
else
	echo "librnd installation not found - try to configure this checkout first or install librnd in /usr or /usr/local" >&2
	exit 1
fi

### generate description.txt (file formats) ###

echo "$RNDV" > auto/ver_librnd_major

. $LIBRND_LIBDIR/devhelpers/awk_on_formats.sh

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

meta_deps="core io-standard io-alien lib-gui librnd${RNDV}-hid-gtk2-gl librnd${RNDV}-hid-gtk2-gdk export export-sim export-extra auto extra cloud doc import-net"

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
for n in $proot/*/*.tmpasm
do
	sed "s@^@$n @" < $n
done
cat extra.digest
) | awk -v "meta_deps=$meta_deps" -v "librnd_pkgs=$librnd_pkgs" -v "librnd_plugins=$librnd_plugins" -v "RNDV=$RNDV" -v "RNDVER=$RNDVER" '
	BEGIN {
		v = split(meta_deps, A, "[ \t]")
		meta_deps = ""
		for(n = 1; n <= v; n++) {
			if (A[n] == "")
				continue
			if ((!(A[n] ~ "^pcb-rnd")) && (!(A[n] ~ "^librnd")))
				A[n] = "pcb-rnd-" A[n]
			if (meta_deps == "")
				meta_deps = A[n]
			else
				meta_deps = meta_deps " " A[n]
		}

		while((getline < "desc") == 1) {
			if ($0 ~ "^@") {
				pkg=$0
				sub("^@", "", pkg)
				getline SHORT[pkg] < "desc"
				continue
			}
			LONG[pkg] = LONG[pkg] $0 " "
		}

		v = split(librnd_pkgs, A, "[ \t]+")
		for(n = 1; n <= v; n++)
			LIBRND_PKG[A[n]] = 1

		v = split(librnd_plugins, A, "[ \t\r\n]+")
		for(n = 1; n <= v; n++)
			if (split(A[n], B, "=") == 2)
				PLUGIN["pcb-rnd-" B[1]] = B[2]
	}

	function fix_dep(dep)
	{
		if ((dep == "") || (dep ~ "^librnd"))
			return dep
		sub("^pcb-rnd-", "", dep)
		if (dep in LIBRND_PKG)
			return "librnd" RNDV "-" dep
		return "pcb-rnd-" dep
	}

	function fix_deps(deps   ,A,n,s,v)
	{
		v = split(deps, A, "[ \t]+")
		s = ""
		for(n = 1; n <= v; n++)
			s = s " " fix_dep(A[n])
		sub("^ ", "", s)
		return s;
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

	($1 ~ "@appendextdeps") {
		pkg=$2
		files=$0
		sub("@appendextdeps[ \t]*[^ \t]*[ \t]", "", files)
		EXTDEPS[pkg] = EXTDEPS[pkg] " " files
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

	($1 ~ "[.]pup$") && ($2 == "$extdeps") {
		tmp=$0
		sub("^[^ \t]*[ \t]*[$]extdeps[ \t]*", "", tmp)
		PUPEXTDEPS[pkg] = PUPEXTDEPS[pkg] " " tmp
	}

	($1 ~ "[.]tmpasm$") && ($3 == "/local/rnd/mod/CONFFILE") {
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

	function uniq(str    ,A,B,v,n,res)
	{
		v = split(str, A)
		for(n = 1; n <= v; n++)
			B[A[n]] = 1
		for(n in B)
			if (res == "")
				res = n
			else
				res = res " " n
		return res
	}

	END {

#		for(plg in PLUGIN_DEP)
#			print "PLUGIN[" plg "] = " PLUGIN[plg] > "/dev/stderr"
#		exit(1)


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

		print "<h3> Librnd minimum version: " RNDVER "</h3>"
		print RNDVER >  "auto/librnd_min_ver"

		print "<h3> Package summary and dependencies </h3>"
		print "<p>"
		print "<table border=1>"
		print "<tr><th> package <th> depends on (packages) <th> consists of (plugins)"

		for(pkg in PKG) {
			if (pkg == "pcb-rnd-core")
				print "<tr><th>" pkg "<td>" fix_deps(PKG_DEP[pkg]) "<td>(builtin: " PKG[pkg] ")"
			else
				print "<tr><th>" pkg "<td>" fix_deps(PKG_DEP[pkg]) "<td>" PKG[pkg]
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

		print "<h3> External dependencies of Ppackages </h3>"
		print "<p>Note: package names differ from distro to distro, this table only approximates the packahge names external dependencies have on your target."
		print "<p>Note: every package that has .so files in it depends on librnd."
		print "<p>"
		print "<table border=1>"
		print "<tr><th> package <th> extneral dependencies"
		for(plg in PLUGIN)
			EXTDEPS[PLUGIN[plg]] = EXTDEPS[PLUGIN[plg]] " " PUPEXTDEPS[plg]
		for(pkg in PKG)
			print "<tr><th>" pkg "<td>" uniq(EXTDEPS[pkg])
		print "</table>"


		print "<p>File prefixes:<ul>"
		print "	<li> $P: plugin install dir (e.g. /usr/lib/pcb-rnd/)"
		print "	<li> $C: conf dir (e.g. /etc/pcb-rnd/)"
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

