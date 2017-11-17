#include <stdlib.h>
#include <assert.h>
#include "digiarty_dvd_info.h"
#include "../../src/titleinfo.h"
#include "../../src/ifodata.h"
#include "../../src/logger.h"

#ifdef QT_QML_DEBUG
#include "../../src/leak_detector_c.h"
#endif

#include "../../src/titleinfo.c"

digiarty_dvd_info *titleinfo_to_digiarty_dvd_info(titles_info_t *titles_info) {
    if (titles_info == NULL || titles_info->nr_of_titles == 0) {
        return NULL;
    }

    digiarty_dvd_info *dvd_info = (digiarty_dvd_info*)calloc(1, sizeof(digiarty_dvd_info));
    if (dvd_info == NULL) {
        return NULL;
    }

    // 统计总时间，用来分析是否是迪斯尼加密
    //判断是不是迪斯尼
    titles_info->disney = true;
    if (titles_info->nr_of_titles == 99) {
        titles_info->disney = true;
    }
    else if (titles_info->nr_of_titles < 20) {
        titles_info->disney = false;
    }
    else {
        uint32_t totle_duration = 0;
        bool workout_dvd = true;
        for (uint32_t i = 0; i < titles_info->nr_of_titles; i++) {
            // 有大于1小时的title，就认为不是健身类
            if (titles_info->titles[i].duration > 3600) {
                workout_dvd = false;
            }

            // 严格判断吧，最多让用户自己选一下
            // 漏判的话可能要出问题。
            if (totle_duration > 8 * 3600) {
                if (!workout_dvd) {
                    titles_info->disney = true;
                    break;
                }
            }
            else {
                // 防止溢出，大于8小时候就不在累加了
                totle_duration += titles_info->titles[i].duration;
            }
        }
    }


    dvd_info->number_of_title = titles_info->nr_of_titles;
    dvd_info->title_info = (digiarty_title_info*)calloc(dvd_info->number_of_title, sizeof(digiarty_title_info));
    if (dvd_info->title_info == NULL) {
        goto error;
    }

    for (uint32_t i=0; i<titles_info->nr_of_titles; i++) {
        _title_info_t *title = &titles_info->titles[i];
        digiarty_title_info* title_info = &dvd_info->title_info[i];


        uint32_t h, m, s;
        h = title->duration;
        s = h % 60; h /= 60;
        m = h % 60; h /= 60;

        title_info->playback_time.hour = h;
        title_info->playback_time.minute = m;
        title_info->playback_time.second = s;
        if (title->fps.num == 2500) {
            title_info->playback_time.fps = fps_25;
        }
        else {
            title_info->playback_time.fps = fps_2997;
        }


        //title_info->number_of_angles = title->nr_of_angles;
        title_info->number_of_angles = 1;
        title_info->video_attributes = (digiarty_video_attributes*)calloc(1, sizeof(digiarty_video_attributes));
        if (title_info->video_attributes == NULL) {
            goto error;
        }

        if (title->video_info.aspect_width == 4) {
            title_info->video_attributes[0].aspect = va_4_3;
        }
        else {
            title_info->video_attributes[0].aspect = va_16_9;
        }


        if (strcmp(title->video_info.standard, "ntsc") == 0) {
            title_info->video_attributes[0].standard = vs_ntsc;
            if (title->video_info.width == 720) {
                title_info->video_attributes[0].resolution_ntsc = vrn_720_480;
            }
            else if (title->video_info.width == 704) {
                title_info->video_attributes[0].resolution_ntsc = vrn_704_480;
            }
            else if (title->video_info.height == 480) {
                title_info->video_attributes[0].resolution_ntsc = vrn_352_480;
            }
            else if (title->video_info.height == 240) {
                title_info->video_attributes[0].resolution_ntsc = vrn_352_240;
            }
        }
        else {
            title_info->video_attributes[0].standard = vs_pal;
            if (title->video_info.width == 720) {
                title_info->video_attributes[0].resolution_pal = vrp_720_576;
            } else if (title->video_info.width == 704) {
                title_info->video_attributes[0].resolution_pal = vrp_704_576;
            } else if (title->video_info.height == 576) {
                title_info->video_attributes[0].resolution_pal = vrp_352_576;
            } else if (title->video_info.height == 288) {
                title_info->video_attributes[0].resolution_pal = vrp_352_288;
            }
        }

        if (title->nr_of_audios > 0) {
            title_info->number_of_audio = title->nr_of_audios;
            title_info->audio_attributes = (digiarty_audio_attributes*)calloc(title_info->number_of_audio, sizeof(digiarty_audio_attributes));
            if (title_info->audio_attributes == NULL) {
                goto error;
            }

            for (uint32_t i=0; i<title_info->number_of_audio; i++) {
                digiarty_audio_attributes *audio = &title_info->audio_attributes[i];

                audio->id = title->audio_infos[i].stream_info.id;
                strcpy(audio->language, title->audio_infos[i].stream_info.language);
                audio->number_of_channels = title->audio_infos[i].nr_of_channels;
                audio->sample_rate = title->audio_infos[i].sample_rate;

                if (strcmp(title->audio_infos[i].stream_info.codec, "ac3") == 0) {
                    audio->coding_mode = acm_ac3;
                }
                else if (strcmp(title->audio_infos[i].stream_info.codec, "mpeg1") == 0) {
                    audio->coding_mode = acm_mpeg1;
                }
                else if (strcmp(title->audio_infos[i].stream_info.codec, "mpeg2") == 0) {
                    audio->coding_mode = acm_mpeg2;
                }
                else if (strcmp(title->audio_infos[i].stream_info.codec, "lpcm") == 0) {
                    audio->coding_mode = acm_lpcm;
                }
                else if (strcmp(title->audio_infos[i].stream_info.codec, "dts") == 0) {
                    audio->coding_mode = acm_dts;
                }
            }
        }

        if (title->nr_of_subpics > 0) {
            title_info->number_of_subtitle = title->nr_of_subpics;
            title_info->subtitle_attributes = (digiarty_subtitle_attributes*)calloc(title_info->number_of_subtitle, sizeof(digiarty_subtitle_attributes));
            if (title_info->subtitle_attributes == NULL) {
                goto error;
            }

            for (uint32_t i=0; i<title_info->number_of_subtitle; i++) {
                digiarty_subtitle_attributes *subtitle = &title_info->subtitle_attributes[i];
                subtitle->id = title->subpics[i].stream_info.id;
                strcpy(subtitle->language, title->subpics[i].stream_info.language);
            }
        }

        if (title->nr_of_chapters) {
            title_info->number_of_chapter = title->nr_of_chapters;
            title_info->chapter_attributes = (digiarty_chapter_attributes*)calloc(title_info->number_of_chapter, sizeof(digiarty_chapter_attributes));
            if (title_info->chapter_attributes == NULL) {
                goto error;
            }

            for (uint32_t i=0; i<title_info->number_of_chapter; i++) {
                digiarty_chapter_attributes *chapter = &title_info->chapter_attributes[i];
                uint32_t h, m, s;
                h = title->chapters[i].duratio;
                s = h % 60; h /= 60;
                m = h % 60; h /= 60;
                chapter->playback_time.hour = h;
                chapter->playback_time.minute = m;
                chapter->playback_time.second = s;
            }
        }
    }

    dvd_info->is_disney_fake = titles_info->disney;
    if (!dvd_info->is_disney_fake) {
        for (uint32_t i = 0; i < dvd_info->number_of_title; i++) {
            dvd_info->title_info[i].is_enable_title = 1;
        }
    }

    return dvd_info;

error:
    if (dvd_info) {
        free_dvd_info(dvd_info);
    }
    return NULL;
}




FDebugLog g_DebugLog = NULL;

#ifndef QT_QML_DEBUG
static void logger_mycallback_t(void *cls, int level, const char *msg) {
    (void)cls;
    (void)level;

    if (g_DebugLog) {
        g_DebugLog(msg);
    }
}
#endif

digiarty_dvd_info* get_dvd_info(const char* dvd_path, int model) {
    digiarty_dvd_info * dvd_info = NULL;
    logger = logger_init();
#ifdef QT_QML_DEBUG
    logger_set_level(logger, LOGGER_DEBUG);
#else
    logger_set_level(logger, LOGGER_INFO);
    logger_set_callback(logger, logger_mycallback_t, NULL);
#endif

    ifo_data_t *ifo_data = init_ifo_data(dvd_path, title_ifo_load, model);
    if (ifo_data) {
        titles_info_t *titles = init_titles_info(ifo_data);
        if (titles) {
            dvd_info = titleinfo_to_digiarty_dvd_info(titles);
            if(dvd_info) {
                title_score(titles, ifo_data);
                int count = 0;
                uint32_t *ts = get_main_title(titles, &count);
                if (ts && count > 0) {
                    //过滤掉时间相同的title
                    uint32_t *durations = (uint32_t*)calloc(count, sizeof(uint32_t));
                    for (int i=0; i<count; i++) {
                        int n = 0;
                        bool find = false;
                        while(true) {
                            if (n >= count) {
                                break;
                            }
                            if (durations[n] == 0) {
                                durations[n] = titles->titles[ts[i]-1].duration;
                                break;

                            }
                            else if (durations[n] == titles->titles[ts[i]-1].duration) {
                                find = true;
                                break;
                            }
                            n++;
                        }

                        if (!find) {
                            if (ts[i] <= dvd_info->number_of_title) {
                                dvd_info->title_info[ts[i]-1].is_enable_title = 1;
                                dvd_info->title_info[ts[i]-1].is_main_title = 1;
                            }
                        }
                    }
                    free(ts);
                    free(durations);
                }
                dvd_info->track_model = get_track_model(ifo_data);
                dvd_info->is_css = get_have_css(ifo_data);
            }
            destroy_titles_info(titles);
        }
        destroy_ifo_data(ifo_data);
    }

    return dvd_info;
}

void free_dvd_info(digiarty_dvd_info* dvd_info) {
    if (dvd_info) {
        if (dvd_info->title_info) {
            for (uint32_t i=0; i<dvd_info->number_of_title; i++) {
                digiarty_title_info *title = &dvd_info->title_info[0];
                if (title->video_attributes) {
                    free(title->video_attributes);
                    title->video_attributes = NULL;
                }

                if (title->audio_attributes) {
                    free(title->audio_attributes);
                    title->audio_attributes= NULL;
                }

                if (title->subtitle_attributes) {
                    free(title->subtitle_attributes);
                    title->subtitle_attributes = NULL;
                }

                if (title->chapter_attributes) {
                    free(title->chapter_attributes);
                    title->chapter_attributes = NULL;
                }
            }

            free(dvd_info->title_info);
            dvd_info->title_info = NULL;
        }
        free(dvd_info);
    }
}

void SetDebugLogFunction(FDebugLog DebugLog) {
    g_DebugLog = DebugLog;
}
