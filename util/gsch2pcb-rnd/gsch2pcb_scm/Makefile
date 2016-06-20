names = c1 c2 c3
schematics = $(names:%=%.sch)
netlists   = $(names:%=%.net)
goldens    = $(names:%=golden/%.net)
diffs      = $(names:%=%.diff)

.PHONY: all clean $(netlists)
all: $(diffs)

$(diffs): %.diff: %.net golden/%.net
	diff $^ > $@

$(netlists): %.net: %.sch
	gnetlist -g gsch2pcb-rnd -o $@ -m ../gnet-gsch2pcb-rnd.scm $<

clean: 
	-rm -rf $(diffs) $(netlists)
