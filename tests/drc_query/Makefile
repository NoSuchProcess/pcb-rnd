ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/drc_query
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1

TESTS = \
thickness.diff overlap.diff

test: $(TESTS)

all:

thickness.diff: thickness.out
	@diff -u thickness.ref thickness.out && rm thickness.out

thickness.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/thickness.lht --gui batch > $(TDIR)/thickness.out

overlap.diff: overlap.out
	@diff -u overlap.ref overlap.out && rm overlap.out

overlap.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/overlap.lht --gui batch > $(TDIR)/overlap.out


clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
