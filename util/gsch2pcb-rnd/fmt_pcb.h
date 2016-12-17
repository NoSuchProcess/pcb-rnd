PcbElement *pcb_element_line_parse(char * line);
void get_pcb_element_list(char * pcb_file);
void prune_elements(char * pcb_file, char * bak);
int add_elements(char * pcb_file);
void update_element_descriptions(char * pcb_file, char * bak);
int insert_element(FILE * f_out, FILE * f_elem, char * footprint, char * refdes, char * value);
void fmt_pcb_init(void);

