# NOTE: aagraph is at svn://repo.hu/projects/aagraph/trunk

CLEANFILES=../action_reference.html ../dialogtable.html ../bridges.svg ../formats.html

all:  ../action_reference.html ../dialogtable.html ../formats.html ../bridges.svg

../action_reference.html : ./dump_actions_to_html.sh ../../../../src/pcb-rnd
	./dump_actions_to_html.sh > ../action_reference.html

../dialogtable.html: pcb-rnd-dialogs ./process_dialogtable.sh ../../../../src/pcb-rnd
	./process_dialogtable.sh > ../dialogtable.html

../bridges.svg: bridges.txt
	aagraph -Tsvg bridges.txt > ../bridges.svg

../formats.html: gen_formats.sh ../../../../src_plugins/io_*/*.pup ../../../../src_plugins/import_*/*.pup ../../../../src_plugins/export_*/*.pup
	./gen_formats.sh > ../formats.html

clean:  
	rm $(CLEANFILES)
