# NOTE: aagraph is at svn://repo.hu/projects/aagraph/trunk

CLEANFILES=../action_reference.html ../dialogtable.html ../bridges.svg ../formats.html

PCBRND = ../../../../src/pcb-rnd

all: ../action_details.html ../action_reference.html ../dialogtable.html ../formats.html ../bridges.svg ../dialogs.html

../action_details.html: ./action_compiler.sh ../action_src/*.html
	./action_compiler.sh ../action_src/*.html > ../action_details.html

../action_reference.html : ./dump_actions_to_html.sh $(PCBRND) ../action_details.html
	./dump_actions_to_html.sh > ../action_reference.html

../dialogtable.html: pcb-rnd-dialogs ./process_dialogtable.sh $(PCBRND)
	./process_dialogtable.sh > ../dialogtable.html

../bridges.svg: bridges.txt
	aagraph -Tsvg bridges.txt > ../bridges.svg

../formats.html: gen_formats.sh ../../../../src_plugins/io_*/*.pup ../../../../src_plugins/import_*/*.pup ../../../../src_plugins/export_*/*.pup
	./gen_formats.sh > ../formats.html

../dialogs.html: $(PCBRND) ./gen_dialogs.sh
	./gen_dialogs.sh > ../dialogs.html

clean:  
	rm $(CLEANFILES)
