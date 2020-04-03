ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/query
PCBRND=./pcb-rnd

TESTS = \
	action.diff getconf.diff getconf2.diff

all: $(TESTS)

action.diff: action.out
	@diff -u action.out action.ref && rm action.out

action.out: FORCE
	@cd $(SRC) && echo 'query(eval, "action(\"message\", \"hello\", @.x)")' | ./$(PCBRND) $(TDIR)/1obj.lht --gui batch > $(TDIR)/action.out

getconf.diff: getconf.out
	@diff -u getconf.out getconf.ref && rm getconf.out

getconf.out: FORCE
	@cd $(SRC) && echo 'query(eval, "@ thus getconf(\"editor/grid_unit\")")' | ./$(PCBRND) $(TDIR)/1obj.lht --gui batch > $(TDIR)/getconf.out

getconf2.diff: getconf2.out
	@diff -u getconf2.out getconf2.ref && rm getconf2.out

getconf2.out: FORCE
	@cd $(SRC) && echo 'query(eval, "@ thus $$min_drill")' | ./$(PCBRND) $(TDIR)/1obj.lht --gui batch > $(TDIR)/getconf2.out


FORCE:
