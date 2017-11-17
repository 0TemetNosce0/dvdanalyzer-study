#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ifodata.h"
#include "logger.h"

#ifdef QT_QML_DEBUG
#include "leak_detector_c.h"
#endif



struct ifo_data_s {
    dvd_reader_t *reader;
    uint32_t nr_of_vtss;
    ifo_handle_t **vtss;//ifo句柄
    dvd_track_model track_model;
    int have_css;
};

void destroy_ifo_data(ifo_data_t *ifo_data) {
    if (ifo_data) {
        if (ifo_data->vtss) {
            for (uint32_t vts_index = 0; vts_index < ifo_data->nr_of_vtss; vts_index++) {
                ifoClose(ifo_data->vtss[vts_index]);
                ifo_data->vtss[vts_index] = NULL;
            }
            free(ifo_data->vtss);
            ifo_data->vtss = NULL;
        }

        if (ifo_data->reader) {
            DVDClose(ifo_data->reader);
            ifo_data->reader = NULL;
        }


        free(ifo_data);
    }
}

ifo_data_t* read_ifo_data(dvd_reader_t *reader, ifo_load_model load_model) {
    ifo_handle_t *video_ts = NULL;
    ifo_data_t *ifo_data = NULL;

    if (reader == NULL) {
        return NULL;
    }

    ifo_data = (ifo_data_t*)calloc(1, sizeof(ifo_data_t));
    if (ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "alloc memory error");
        return NULL;
    }


    video_ts = ifoOpen(reader, 0);//读取ifo。0是video manager IFO file，1是title1
    if (video_ts == NULL) {
        logger_log(logger, LOGGER_ERR, "can not find video_ts.ifo");
        goto error;
    }

    if (video_ts->vts_atrt == NULL) {////视频标题集属性表
        logger_log(logger, LOGGER_ERR, "ifo file error : vts_atrt == NULL");
        goto error;
    }

    uint32_t nr_of_vtss = video_ts->vts_atrt->nr_of_vtss;//ifo，0只有video manager IFO file，1有两个ifo
    if (nr_of_vtss == 0) {
        logger_log(logger, LOGGER_ERR, "ifo file error : number of vts is 0");
        goto error;
    }

    ifo_data->nr_of_vtss = nr_of_vtss + 1;
    ifo_data->vtss = (ifo_handle_t**)calloc(ifo_data->nr_of_vtss, sizeof(ifo_handle_t*));
    if (ifo_data->vtss == NULL) {
        logger_log(logger, LOGGER_ERR, "alloc memory error");
        goto error;
    }

    if (load_model == title_ifo_load) {//只读取有标题的ifo信息
        //打开包含title信息的ifo文件
        for (uint32_t title_index = 0; title_index < video_ts->tt_srpt->nr_of_srpts; title_index++) {
            title_info_t *title = &video_ts->tt_srpt->title[title_index];
            if (title->title_set_nr > ifo_data->nr_of_vtss) {
                logger_log(logger, LOGGER_ERR, "ifo file error : title_set_nr > nr_of_vtss");
                goto error;
            }

            if (ifo_data->vtss[title->title_set_nr] == NULL) {
                ifo_data->vtss[title->title_set_nr] = ifoOpen(reader, title->title_set_nr);//打开对应的标题title->title_set_nr
            }
        }
    }
    else if (load_model == all_ifo_load) {//读取所有信息
        for (uint32_t vts_index = 1; vts_index <= nr_of_vtss; vts_index++) {
            if (ifo_data->vtss[vts_index] == NULL) {
                ifo_data->vtss[vts_index] = ifoOpen(reader, vts_index);
            }
        }
    }

    ifo_data->reader = reader;
    ifo_data->vtss[0] = video_ts;
    video_ts = NULL;

    return ifo_data;

error:

    if (video_ts) {
        ifoClose(video_ts);
        video_ts = NULL;
    }

    destroy_ifo_data(ifo_data);
    return NULL;
}

ifo_data_t* init_ifo_data(const char* path, ifo_load_model load_model, int read_model) {
    ifo_data_t *ifo_data = NULL;
    dvd_reader_t *reader = NULL;

    if (path == NULL || strlen(path) == 0) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return NULL;
    }

    reader = DVDOpen(path);
    if (reader == NULL) {
        logger_log(logger, LOGGER_ERR, "can not open dvd %s", path);
        DVDClose(reader);
        return NULL;
    }

    if (read_model == 0) {
        //首先使用udf模式
        Set_DVDRead_UDFISO9600(1);//设置模式：UDF =1 ; ISO9600 = 2
        ifo_data = read_ifo_data(reader, load_model);//读取ifo，
        if (ifo_data == NULL) {
            //再次使用iso读取
            Set_DVDRead_UDFISO9600(2);
            ifo_data = read_ifo_data(reader, load_model);
            if (ifo_data == NULL) {
                return NULL;
            }
            else {
                ifo_data->track_model = iso_track;
            }
        }
        else {
            ifo_data->track_model = udf_track;
        }
    }
    else {
        if (read_model != 1 && read_model != 2) {
            return NULL;
        }

        Set_DVDRead_UDFISO9600(read_model);
        ifo_data = read_ifo_data(reader, load_model);
        if (ifo_data == NULL) {
            return NULL;
        }
        else {
            ifo_data->track_model = read_model;
        }
    }
    //读取一下VIDEO_TS.VOB文件，获取是不是使用了css。

    dvd_file_t *vob = DVDOpenFile(ifo_data->reader, 0, DVD_READ_MENU_VOBS);//vob读取
    if (vob) {
        DVDCloseFile(vob);
        vob = NULL;
    }

    ifo_data->have_css = Get_DVDDiscisEncrypted();//读取的文件是否加密了。

    return ifo_data;
}

dvd_track_model get_track_model(ifo_data_t *ifo_data) {
    if (ifo_data) {
        return ifo_data->track_model;
    }
    return unknow_track;
}

bool get_have_css(ifo_data_t *ifo_data) {
    if (ifo_data && ifo_data->have_css != 0) {
        return true;
    }
    return false;
}

ifo_handle_t* get_ifo_data(ifo_data_t *ifo_data, uint32_t vts_index) {
    if (ifo_data == NULL || ifo_data->reader == NULL || ifo_data->nr_of_vtss <= vts_index) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return NULL;
    }

    if (vts_index == 0) {
        return ifo_data->vtss[vts_index];
    }
    else if (ifo_data->vtss[vts_index] == NULL){
        ifo_data->vtss[vts_index] = ifoOpen(ifo_data->reader, vts_index);
    }
    return ifo_data->vtss[vts_index];
}

const char* get_volume(ifo_data_t *ifo_data) {
    static char volume[33];
    static unsigned char unused[1024];

    if (ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return NULL;
    }

    //卷标信息
    if (DVDUDFVolumeInfo(ifo_data->reader, volume, sizeof(volume), unused, sizeof(unused)) < 0) {
        if (DVDISOVolumeInfo(ifo_data->reader, volume, sizeof(volume), unused, sizeof(unused)) < 0) {
            return NULL;
        }
    }

    return volume;
}

uint32_t get_title_by_vts(ifo_data_t *ifo_data, uint32_t vts, uint32_t ttn) {
    if (ifo_data == NULL || vts == 0 || ttn == 0) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return 0;
    }

    ifo_handle_t *video_ts = get_ifo_data(ifo_data, 0);
    if (video_ts == NULL) {
        logger_log(logger, LOGGER_ERR, "cannot get video_ts handle", __func__);
        return 0;
    }

    for (uint32_t title_index = 0; title_index < video_ts->tt_srpt->nr_of_srpts; title_index++) {
        title_info_t *title = &video_ts->tt_srpt->title[title_index];
        if (title->title_set_nr == vts && title->vts_ttn == ttn) {
            return title_index + 1;
        }
    }
    return 0;
}

bool get_vts_ttn_by_title(ifo_data_t *ifo_data, uint32_t title_index, uint32_t *vts, uint32_t *ttn) {
    if (ifo_data == NULL || title_index == 0 || vts == NULL || ttn == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return false;
    }

    *vts = 0;
    *ttn = 0;

    ifo_handle_t *video_ts = get_ifo_data(ifo_data, 0);
    if (video_ts == NULL) {
        logger_log(logger, LOGGER_ERR, "cannot get video_ts handle", __func__);
        return false;
    }

    if (title_index > video_ts->tt_srpt->nr_of_srpts) {
        return false;
    }

    title_info_t *title = &video_ts->tt_srpt->title[title_index - 1];
    *vts = title->title_set_nr;
    *ttn = title->vts_ttn;

    return true;
}

uint32_t get_vts_number(ifo_data_t *ifo_data) {
    if (ifo_data == NULL) {
        return 0;
    }

    if (ifo_data->nr_of_vtss == 0) {
        return 0;
    }

    return ifo_data->nr_of_vtss - 1;
}

uint32_t get_framerate(dvd_time_t *dt) {
    static int framerates[4] = {0, 2500, 0, 2997};
    return framerates[(dt->frame_u & 0xc0) >> 6];
}

uint32_t playbacktimetoframe(dvd_time_t *dt) {
    static int framerates[4] = {0, 2500, 0, 2997};
    int framerate = framerates[(dt->frame_u & 0xc0) >> 6];
    int msec = (((dt->hour & 0xf0) >> 3) * 5 + (dt->hour & 0x0f)) * 3600;
    msec += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60;
    msec += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f));
    if(framerate > 0) {
        return msec * framerate + (((dt->frame_u & 0x30) >> 3) * 5 + (dt->frame_u & 0x0f))*100;
    }
    return 0;
}

uint32_t playbacktimetosec(dvd_time_t *dt) {
    static int framerates[4] = {0, 2500, 0, 2997};
    int framerate = framerates[(dt->frame_u & 0xc0) >> 6];
    int msec = (((dt->hour & 0xf0) >> 3) * 5 + (dt->hour & 0x0f)) * 3600;
    msec += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60;
    msec += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f));
    if(framerate > 0) {
        msec += (((dt->frame_u & 0x30) >> 3) * 5 + (dt->frame_u & 0x0f)) * 100 / framerate;
    }
    return msec;
}


const char* sec_to_timestring(uint32_t duration) {
    static char buf[] = "00:00:00";
    int h, m, s;
    h = duration / 3600;
    m = (duration % 3600) / 60;
    s = duration % 60;

    sprintf(buf, "%02d:%02d:%02d", h, m, s);
    return buf;
}


