TESTS = C.diff H.diff H2.diff H3.diff H4.diff O.diff \
        rats1.diff rats2.diff rats3.diff

all: $(TESTS)

.SUFFIXES: .in .out .diff .ref

.out.diff:
	@diff -u $*.out $*.ref

.in.out:
	@../main < $*.in >$*.out
