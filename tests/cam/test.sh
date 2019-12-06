#!/bin/sh
#DBG=cdb

TESTS="grp_name type offs"

do_test()
{
	local testcase="$1" refs r o
	$DBG pcb-rnd -C $testcase/cam.conf -x cam test --outfile $testcase/out layers.lht

	(
		cd $testcase
		refs=`ls ref.*.svg 2>/dev/null`
		if test -z "$refs"
		then
			tar -zxf ref.tar.gz
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
