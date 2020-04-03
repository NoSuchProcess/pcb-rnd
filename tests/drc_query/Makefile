ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/drc_query
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1

TESTS = \
thickness.diff overlap.diff netint.diff dwg_area.diff

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

netint.diff: netint.out
	@diff -u netint.ref netint.out && rm netint.out

netint.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/netint.lht --gui batch > $(TDIR)/netint.out

dwg_area.diff: dwg_area.out
	@diff -u dwg_area.ref dwg_area.out && rm dwg_area.out

dwg_area.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/dwg_area.lht --gui batch > $(TDIR)/dwg_area.out


clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
