PCBRND=pcb-rnd
CLEANFILES=../action_reference.html ../dialogtable.html

all:  action_reference.html dialogtable.html


action_reference.html: dump_actions_to_html.sh
	./dump_actions_to_html.sh > ../action_reference.html

dialogtable.html:  pcb-rnd-dialogs
	./process_dialogtable.sh > ../dialogtable.html

clean:  
	rm $(CLEANFILES)
