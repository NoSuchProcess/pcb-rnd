GFLT=../pupfilter.sh ws

all:

test:
	@./test_patch.sh insert.lht 'MenuPatch(load, test, "insert.lht");' | $(GFLT) > insert.out
	@diff -w insert.ref insert.out
	@./test_patch.sh change.lht 'MenuPatch(load, test, "change.lht");' | $(GFLT) > change.out
	@diff -w change.ref change.out
	@./test_patch.sh remove.lht 'MenuPatch(load, test, "remove.lht");' | $(GFLT) > remove.out
	@diff -w remove.ref remove.out
	@./test_patch.sh overwrite_m.lht 'MenuPatch(load, test, "overwrite_m.lht");' | $(GFLT) > overwrite_m.out
	@diff -w overwrite_m.ref overwrite_m.out
	@./test_patch.sh overwrite_p.lht 'MenuPatch(load, test, "overwrite_p.lht");' | $(GFLT) > overwrite_p.out
	@diff -w overwrite_p.ref overwrite_p.out
	@./test_patch.sh new_submenu.lht 'MenuPatch(load, test, "new_submenu.lht");' | $(GFLT) > new_submenu.out
	@diff -w new_submenu.ref new_submenu.out

clean:
	touch dummy.out
	rm *.out
