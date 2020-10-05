all:

test:
	@./test_patch.sh 'MenuPatch(load, test, "insert.lht");' > insert.out
	@diff insert.ref insert.out