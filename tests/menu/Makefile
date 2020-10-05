all:

test:
	@./test_patch.sh 'MenuPatch(load, test, "insert.lht");' > insert.out
	@diff insert.ref insert.out
	@./test_patch.sh 'MenuPatch(load, test, "change.lht");' > change.out
	@diff change.ref change.out
	@./test_patch.sh 'MenuPatch(load, test, "remove.lht");' > remove.out
	@diff remove.ref remove.out
	@./test_patch.sh 'MenuPatch(load, test, "overwrite_m.lht");' > overwrite_m.out
	@diff overwrite_m.ref overwrite_m.out
	@./test_patch.sh 'MenuPatch(load, test, "overwrite_p.lht");' > overwrite_p.out
	@diff overwrite_p.ref overwrite_p.out
	@./test_patch.sh 'MenuPatch(load, test, "new_submenu.lht");' > new_submenu.out
	@diff new_submenu.ref new_submenu.out

clean:
