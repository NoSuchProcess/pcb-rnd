all: tree.html tree.txt

tree.html: render.sh *.lht
	./render.sh > tree.html

tree.txt: tree.html
	w3m -cols 256 tree.html > tree.txt

comm_flags.lht: ../../../src/pcb-rnd render/gen_flags.sh
	cd render && ./gen_flags.sh > ../comm_flags.lht