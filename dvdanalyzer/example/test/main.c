#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ifodata.h"
#include "../src/titleinfo.h"
#include "../src/pgcanalyzer.h"
#include "../src/logger.h"

void print_main_title(const char* volume, int count, int *titles) {
    if (count > 0 && titles != NULL) {
        printf("{\n");
        if (volume != NULL) {
            printf("\"Volume\" : \"%s\",\n", volume);
        }
        else {
            printf("\"Volume\" : \"UNKNOW\",\n");
        }
        printf("\"Titles\" : [\n");
        for (int i=0; i<count; i++) {
            printf("%d", titles[i]);
            if (i+1 == count) {
                printf("\n");
            }
            else {
                printf(",\n");
            }
        }

        printf("]\n");
        printf("}\n");

    }
}




int main(int argc, char *argv[])
{
    //    if (argc < 2) {
    //        return 1;
    //    }

    //    bool detailed = true;
    //    if (argc == 3) {
    //        detailed = true;
    //    }


    logger = logger_init();
    //    logger_set_level(logger, LOGGER_DEBUG);
    logger_set_level(logger, LOGGER_INFO);

    if (1/*strcmp(argv[2], "title") == 0*/) {//E:/201503/VIDEO_TS  H:/
        ifo_data_t *ifo_data = init_ifo_data("E:/201503/MIB II.iso"/*argv[1]*/, title_ifo_load, atoi(0/*argv[3]*/));
        if (ifo_data) {
            const char *volume = get_volume(ifo_data);

            titles_info_t *titles = init_titles_info(ifo_data);
            if (titles) {
                title_score(titles, ifo_data);
                //                fflush(stderr);
                //                fflush(stdout);
                //                printf_titles_info(titles, detailed);
                //                fflush(stderr);
                //                fflush(stdout);
                int count = 0;
                int *ts = get_main_title(titles, &count);
                printf("11111111111111111,%s\n",volume);
                print_main_title(volume, count, ts);//打印


                free(ts);
                destroy_titles_info(titles);
            }
            destroy_ifo_data(ifo_data);
        }
    }
    else if (strcmp(argv[2], "pgc") == 0) {
        ifo_data_t *ifo_data = init_ifo_data(argv[1], all_ifo_load, atoi(argv[3]));
        if (ifo_data) {
            const char *volume = get_volume(ifo_data);
            pgc_analyzer_t *analyzer = pgc_analyzer(ifo_data);
            if (analyzer) {
                int count = 0;
                int *ts = get_main_title_by_pgc(analyzer, &count);

                print_main_title(volume, count, ts);

                free(ts);
            }
        }
    }
    else if (strcmp(argv[2], "nav") == 0) {

    }

    logger_destroy(logger);
    logger = NULL;
    return 0;
}
