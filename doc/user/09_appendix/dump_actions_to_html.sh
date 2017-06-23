#! /bin/bash
# bash script to dump pcb-rnd actions and parse output a html table for the
# docs
#
#errata: pcb_dump_actions function in src/hid_actions.c
	#printf("A%s\n", ca->action->name);
	#dump_string('D', desc);
	#dump_string('S', synt);

# we want to sort the pcb-rnd actions and process them into a captioned html
# table with three header columns: Action Name, Description, Syntax 
#
# if the line starts with "A", strip the letter and enclose the rest of the
# line in a <tr> tag
# if the next line starts with a D, strip the letter and enclose the rest of
# the line in a <td> tag, add newline and start the <td> tag for any syntax content
# any following lines that start with an S are printed in the <td> 
# repeat

#TODO: use pcb-rnd version used to generate the table in table header 
#TODO: fix escape parsing in syntax sections 

cat table_opener.html > action_reference.html 
pcb-rnd --dump-actions | sed -e 's/\(^.\)/\1 /g' >> actions.list
awk '{
  if ($1 == "A")
    {printf "<tr><td> %s </td>\n", $2 }
  else if ($1 == "D")
    {printf "<td> %s </td>\n<td>", $2 }
  else if ($1 == "S")
    {printf " %s ,", $2 }
}' actions.list >> action_reference.html


