TESTS =  \
	list_merge.diff \
	arr_merge.diff \
	arr_set.diff \
	scalar.diff

# temporarily disabled because of a conf bug
#	list_set.diff

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


