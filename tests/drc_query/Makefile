ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/drc_query
PCBRND=./pcb-rnd

all: thickness.diff

thickness.diff: thickness.out
	@diff -u thickness.out thickness.ref && rm thickness.out

thickness.out: FORCE
	@cd $(SRC) && echo "drc()" | ./$(PCBRND) $(TDIR)/thickness.lht --gui batch > $(TDIR)/thickness.out

overlap.diff: overlap.out
	@diff -u overlap.out overlap.ref && rm overlap.out

overlap.out: FORCE
	@cd $(SRC) && echo "drc()" | ./$(PCBRND) $(TDIR)/overlap.lht --gui batch > $(TDIR)/overlap.out


FORCE:
