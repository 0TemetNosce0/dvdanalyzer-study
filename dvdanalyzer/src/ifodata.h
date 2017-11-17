#ifndef IFODATA_H
#define IFODATA_H

#include <stdbool.h>
#include <dvdread/ifo_read.h>

typedef struct ifo_data_s ifo_data_t;

typedef enum {
    none_ifo_load,
    all_ifo_load,
    title_ifo_load
} ifo_load_model;

typedef enum {
    unknow_track = -1,
    not_checked_track = 0,
    udf_track,
    iso_track
} dvd_track_model;

ifo_data_t* init_ifo_data(const char* path, ifo_load_model load_model, int read_model);
void destroy_ifo_data(ifo_data_t *ifo_data);
ifo_handle_t* get_ifo_data(ifo_data_t *ifo_data, uint32_t vts_index);
const char* get_volume(ifo_data_t *ifo_data);
dvd_track_model get_track_model(ifo_data_t *ifo_data);
bool get_have_css(ifo_data_t *ifo_data);

uint32_t get_title_by_vts(ifo_data_t *ifo_data, uint32_t vts, uint32_t ttn);
bool get_vts_ttn_by_title(ifo_data_t *ifo_data, uint32_t title_index, uint32_t *vts, uint32_t *ttn);

uint32_t get_vts_number(ifo_data_t *ifo_data);
uint32_t get_framerate(dvd_time_t *dt);
uint32_t playbacktimetoframe(dvd_time_t *dt);
uint32_t playbacktimetosec(dvd_time_t *dt);
const char* sec_to_timestring(uint32_t duration);


#endif // IFODATA_H
