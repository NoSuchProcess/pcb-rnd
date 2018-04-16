TESTS =  \
	list_merge.diff \
	list_set.diff \
	arr_merge.diff \
	scalar.diff

# temporarily disabled because of a conf bug
#	arr_set.diff

.SUFFIXES: .ref .test .out .diff .REF

all: $(TESTS)
	@echo "conf:                *** QC PASS ***"

.test.out:
	@../conftest < $*.test >$*.out

.test.REF:
	@../conftest < $*.test >$*.ref
	@echo $*.ref generated, please validate the content

.out.diff:
	@diff -u $*.ref $*.out


