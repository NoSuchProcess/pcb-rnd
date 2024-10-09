ROOT=../..
include $(ROOT)/Makefile.conf

SRC=$(ROOT)/src
TDIR=../tests/pstk_crescent
GFLT=grep -v "<anon>" | sort
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1 -c debug/trace_pstk_crescent=1  --gui batch

TESTS = \
	line-circ.diff \
	qline-circ.diff \
	circ-circ.diff \
	poly-circ.diff

test: $(TESTS)

all:

line-circ.diff: line-circ.out
	@diff -u line-circ.ref line-circ.out && rm line-circ.out

line-circ.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/line-circ.rp | $(GFLT) > $(TDIR)/line-circ.out

qline-circ.diff: qline-circ.out
	@diff -u qline-circ.ref qline-circ.out && rm qline-circ.out

qline-circ.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/qline-circ.rp | $(GFLT) > $(TDIR)/qline-circ.out

circ-circ.diff: circ-circ.out
	@diff -u circ-circ.ref circ-circ.out && rm circ-circ.out

circ-circ.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/circ-circ.rp | $(GFLT) > $(TDIR)/circ-circ.out

poly-circ.diff: poly-circ.out
	@diff -u poly-circ.ref poly-circ.out && rm poly-circ.out

poly-circ.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/poly-circ.rp | $(GFLT) > $(TDIR)/poly-circ.out

clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
