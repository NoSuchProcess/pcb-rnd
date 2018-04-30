DOTS = \
 data.dot \
 geda-project-v1.dot \
 pcb-rnd-board-v.dot \
 pcb-rnd-conf-v1.dot \
 pcb-rnd-font-v1.dot \
 pcb-rnd-subcircuit-v.dot \
 submenu.dot

all: tree.html tree.txt $(DOTS)

%.dot: render/render.sh render/*.awk *.lht
	cd render && ./render.sh dot
	for n in *.dot; do dot -Tsvg $$n > $${n%%.dot}.svg; done

tree.html: render/render.sh render/*.awk *.lht
	cd render && ./render.sh html > ../tree.html

tree.txt: tree.html
	w3m -cols 256 tree.html > tree.txt

comm_flags.lht: ../../../src/pcb-rnd render/gen_flags.sh
	cd render && ./gen_flags.sh > ../comm_flags.lht
