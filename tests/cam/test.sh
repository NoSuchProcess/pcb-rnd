#!/bin/sh
ROOT=../..
#DBG=cdb

TESTS="grp_name type offs"
PWD=`pwd`

run_pcb_rnd()
{
	(cd $ROOT/src && $DBG ./pcb-rnd "$@") 2>&1 | awk '
		/^[*][*][*] Exporting:/ { next }
		/Warning: footprint library list error on/ { next }
		/^[ \t]*$/ { next }
		{ print $0 }
	'
}

do_test()
{
	local testcase="$PWD/$1" refs r o
	run_pcb_rnd -C $testcase/cam.conf -x cam test --outfile $testcase/out $PWD/layers.lht

	(
		cd $testcase
		refs=`ls ref.*.svg 2>/dev/null`
		if test -z "$refs"
		then
			cp ref.tar.gz unpack.tar.gz
			gzip -d unpack.tar.gz
			tar -xf unpack.tar
			rm unpack.tar
		fi
		for r in ref.*.svg
		do
			o=out.${r#ref.}
			diff -u $r $o
		done
	)
}

do_clean()
{
	local testcase="$1"
	rm -f $testcase/ref.*.svg $testcase/out.*.svg
}

cmd="$1"
shift 1

tests="$@"
if test -z "$tests"
then
	tests=$TESTS
fi

for n in $tests
do
	case "$cmd" in
		test) do_test $n ;;
		clean) do_clean $n ;;
	esac
done
