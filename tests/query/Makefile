ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/query
PCBRND=./pcb-rnd
GLOBARGS=-c rc/library_search_paths=../tests/RTT/lib -c rc/quiet=1

TESTS = \
	action.diff getconf.diff getconf2.diff

test: $(TESTS)

all:

action.diff: action.out
	@diff -u action.ref action.out && rm action.out

action.out: FORCE
	@cd $(SRC) && echo 'query(eval, "action(\"message\", \"hello\", @.x)")' | $(PCBRND) $(GLOBARGS) $(TDIR)/1obj.lht --gui batch > $(TDIR)/action.out

getconf.diff: getconf.out
	@diff -u getconf.ref getconf.out && rm getconf.out

getconf.out: FORCE
	@cd $(SRC) && echo 'query(eval, "@ thus getconf(\"editor/grid_unit\")")' | $(PCBRND) $(GLOBARGS) $(TDIR)/1obj.lht --gui batch > $(TDIR)/getconf.out

getconf2.diff: getconf2.out
	@diff -u getconf2.ref getconf2.out && rm getconf2.out

getconf2.out: FORCE
	@cd $(SRC) && echo 'query(eval, "@ thus $$min_drill")' | $(PCBRND) $(GLOBARGS) $(TDIR)/1obj.lht --gui batch > $(TDIR)/getconf2.out

clean:
	@echo "a" > dummy.out
	rm *.out

FORCE:
