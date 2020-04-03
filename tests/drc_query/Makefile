ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/drc_query
PCBRND=./pcb-rnd

all: thickness.diff

thickness.diff: thickness.out
	@diff -u thickness.out thickness.ref && rm thickness.out

thickness.out: FORCE
	@cd $(SRC) && echo "drc()" | ./$(PCBRND) $(TDIR)/thickness.lht --gui batch > $(TDIR)/thickness.out


FORCE:
