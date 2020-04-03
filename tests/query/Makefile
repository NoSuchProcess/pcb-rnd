ROOT=../..

SRC=$(ROOT)/src
TDIR=../tests/query
PCBRND=./pcb-rnd

all: action.diff

action.diff: action.out
	@diff -u action.out action.ref && rm action.out

action.out: FORCE
	cd $(SRC) && echo 'query(eval, "action(\"message\", \"hello\", @.x)")' | ./$(PCBRND) $(TDIR)/1obj.lht --gui batch > $(TDIR)/action.out


FORCE:
