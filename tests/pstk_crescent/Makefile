ROOT=../..
include $(ROOT)/Makefile.conf

SRC=$(ROOT)/src
TDIR=../tests/pstk_crescent
GFLT=grep -v "<anon>" | grep " n=0 " | sort
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1 -c debug/trace_pstk_crescent=1  --gui batch

TESTS = \
	line-circ.diff \
	line-line.diff \
	line-qline.diff \
	poly-line.diff \
	poly-qline.diff \
	qline-circ.diff \
	qline-qline.diff \
	circ-circ.diff \
	poly-circ.diff

test: $(TESTS)

all:

line-circ.diff: line-circ.out
	@diff -u line-circ.ref line-circ.out && rm line-circ.out

line-circ.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/line-circ.rp | $(GFLT) > $(TDIR)/line-circ.out

line-line.diff: line-line.out
	@diff -u line-line.ref line-line.out && rm line-line.out

line-line.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/line-line.rp | $(GFLT) > $(TDIR)/line-line.out

line-qline.diff: line-qline.out
	@diff -u line-qline.ref line-qline.out && rm line-qline.out

line-qline.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/line-qline.rp | $(GFLT) > $(TDIR)/line-qline.out

qline-qline.diff: qline-qline.out
	@diff -u qline-qline.ref qline-qline.out && rm qline-qline.out

qline-qline.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/qline-qline.rp | $(GFLT) > $(TDIR)/qline-qline.out

poly-line.diff: poly-line.out
	@diff -u poly-line.ref poly-line.out && rm poly-line.out

poly-line.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/poly-line.rp | $(GFLT) > $(TDIR)/poly-line.out

poly-qline.diff: poly-qline.out
	@diff -u poly-qline.ref poly-qline.out && rm poly-qline.out

poly-qline.out: FORCE
	@cd $(SRC) && echo "" | $(PCBRND) $(GLOBARGS) $(TDIR)/poly-qline.rp | $(GFLT) > $(TDIR)/poly-qline.out

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
