ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/query
PCBRND=./pcb-rnd

all: action.diff getconf.diff

action.diff: action.out
	@diff -u action.out action.ref && rm action.out

action.out: FORCE
	@cd $(SRC) && echo 'query(eval, "action(\"message\", \"hello\", @.x)")' | ./$(PCBRND) $(TDIR)/1obj.lht --gui batch > $(TDIR)/action.out

getconf.diff: getconf.out
	@diff -u getconf.out getconf.ref && rm getconf.out

getconf.out: FORCE
	@cd $(SRC) && echo 'query(eval, "@ thus getconf(\"editor/grid_unit\")")' | ./$(PCBRND) $(TDIR)/1obj.lht --gui batch > $(TDIR)/getconf.out


FORCE:
