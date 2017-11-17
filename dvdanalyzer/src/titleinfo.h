#ifndef TITLEINFO_H
#define TITLEINFO_H

#include "ifodata.h"

typedef struct titles_info_s titles_info_t;

titles_info_t* init_titles_info(ifo_data_t *ifo_data);
void destroy_titles_info(titles_info_t *titles_info);

void title_score(titles_info_t *titles_info, ifo_data_t *ifo_data);
void printf_titles_info(titles_info_t *titles_info, bool detailed);

uint32_t* get_main_title(titles_info_t *title_info, int* count);
#endif // TITLEINFO_H
