ROOT=../..
include $(ROOT)/Makefile.conf

SRC=$(ROOT)/src
TDIR=../tests/drc_query
FLT=$(TDIR)/drc_filter.sh
GFLT=$(TDIR)/../pupfilter.sh
PCBRND=./pcb-rnd
GLOBARGS=--gui batch -c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1 -c editor/drc_inclusive_bbox=1

TESTS = \
thickness.diff overlap.diff netint.diff dwg_area.diff fullpoly.diff \
zone_clr.diff invchar.diff poly-poly.diff

test: $(TESTS)

all:

thickness.diff: thickness.out
	@diff -u thickness.ref thickness.out && rm thickness.out

thickness.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/thickness.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/thickness.out

overlap.diff: overlap.out
	@diff -u overlap.ref overlap.out && rm overlap.out

overlap.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/overlap.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/overlap.out

netint.diff: netint.out
	@diff -u netint.ref netint.out && rm netint.out

netint.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/netint.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/netint.out

dwg_area.diff: dwg_area.out
	@diff -u dwg_area.ref dwg_area.out && rm dwg_area.out

dwg_area.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/dwg_area.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/dwg_area.out

fullpoly.diff: fullpoly.out
	@diff -u fullpoly.ref fullpoly.out && rm fullpoly.out

fullpoly.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/fullpoly.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/fullpoly.out

zone_clr.diff: zone_clr.out
	@diff -u zone_clr.ref zone_clr.out && rm zone_clr.out

zone_clr.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/zone_clr.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/zone_clr.out

invchar.diff: invchar.out
	@diff -u invchar.ref invchar.out && rm invchar.out

invchar.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/invchar.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/invchar.out

poly-poly.diff: poly-poly.out
	@diff -u poly-poly.ref poly-poly.out && rm poly-poly.out

poly-poly.out: FORCE
	@cd $(SRC) && echo "drc()" | $(PCBRND) $(GLOBARGS) $(TDIR)/poly-poly.lht --gui batch | $(GFLT) | $(FLT) > $(TDIR)/poly-poly.out

clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
