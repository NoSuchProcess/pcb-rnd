ROOT=../..
include $(ROOT)/Makefile.conf

SRC=$(ROOT)/src
TDIR=../tests/pstk_crescent
GFLT=grep -v "<anon>" | sort
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1 -c debug/trace_pstk_crescent=1  --gui batch

TESTS = \
	line-circ.diff

test: $(TESTS)
	@echo "pstk_crescent: *** QC PASS ***"

all:

line-circ.diff: line-circ.out
	@diff -u line-circ.ref line-circ.out && rm line-circ.out

line-circ.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/line-circ.rp | $(GFLT) > $(TDIR)/line-circ.out

clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
