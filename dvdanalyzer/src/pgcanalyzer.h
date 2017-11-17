#ifndef PGCANALYZER_H
#define PGCANALYZER_H

#include <stdbool.h>
#include "ifodata.h"

typedef struct pgc_analyzer_s pgc_analyzer_t;

pgc_analyzer_t* pgc_analyzer(ifo_data_t *dvd_info);
void destroy_pgc_analyzer(pgc_analyzer_t *pgc_analyzer);
int *get_main_title_by_pgc(pgc_analyzer_t *analyzer, int *count);
#endif // PGCANALYZER_H
