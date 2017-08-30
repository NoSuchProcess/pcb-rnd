#ifndef PCB_LHT_TEMPLATE_H
#define PCB_LHT_TEMPLATE_H
#include <stdio.h>
#include <liblihata/dom.h>

typedef int (*lht_temp_insert_t)(FILE *f, const char *prefix, char *name, lht_err_t *err);

int lht_temp_exec(FILE *f, const char *prefix, lht_doc_t *doc, const char *name, lht_temp_insert_t ins, lht_err_t *err);

#endif
