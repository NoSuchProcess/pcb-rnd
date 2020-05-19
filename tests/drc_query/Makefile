ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/drc_query
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1

TESTS = \
thickness.diff overlap.diff netint.diff dwg_area.diff fullpoly.diff \
zone_clr.diff

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

fullpoly.diff: fullpoly.out
	@diff -u fullpoly.ref fullpoly.out && rm fullpoly.out

fullpoly.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/fullpoly.lht --gui batch > $(TDIR)/fullpoly.out

zone_clr.diff: zone_clr.out
	@diff -u zone_clr.ref zone_clr.out && rm zone_clr.out

zone_clr.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/zone_clr.lht --gui batch > $(TDIR)/zone_clr.out

clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
