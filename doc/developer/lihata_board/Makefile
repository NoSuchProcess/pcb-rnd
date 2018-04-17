all: tree.html tree.txt

tree.html: render.sh tree.lht
	./render.sh > tree.html

tree.txt: tree.html
	w3m -cols 256 tree.html > tree.txt