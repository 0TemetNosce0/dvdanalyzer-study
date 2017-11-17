#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "list.h"
#include "titleinfo.h"
#include "logger.h"

#ifdef QT_QML_DEBUG
#include "leak_detector_c.h"
#endif

#define MAX_LANGUAGE_LEN 4
#define MAX_CODEC_LEN 10
#define MAX_STANDARD_LEN 5

typedef struct {
    uint32_t num;
    uint32_t den;
} _fraction_t;

typedef struct {
    uint32_t id;
    char language[MAX_LANGUAGE_LEN];//语言
    char codec[MAX_CODEC_LEN];//编码：mpeg1，mpeg2，dts(DTS音轨)，lpcm，ac3
} _stream_info_t;

typedef enum {
    reserved_subp,
    not_specified_subp,
    normal_size_subp,
    bigger_size_subp,
    children_sub,
    normal_size_closed_subp,
    bigger_size_closed_subp,
    children_closed_subp,
    forced_subp,
    normal_director_subp,
    bigger_director_subp,
    director_for_children_sub,
} _subp_lang_extension;

typedef struct {
    _stream_info_t stream_info;
    _subp_lang_extension lang_extension;
} _subpic_info_t;

typedef enum {
    reserved_audio,
    not_specified_audio,
    normal_audio,
    visually_impaired_audio,
    director_comments_audio,
} _audio_lang_extension;

typedef struct {
    _stream_info_t stream_info;//流相关信息
    uint32_t sample_rate;//采样率
    char quantization[8];//量化，就是把经过抽样得到的瞬时值将其幅度离散，即用一组规定的电平，把瞬时抽样值用最接近的电平值来表示。
    uint32_t nr_of_channels;//通道数
    _audio_lang_extension lang_extension;
} _audio_info_t;

typedef struct {
    _stream_info_t stream_info;//语言，编码信息
    char standard[MAX_STANDARD_LEN];//制式：PAL，NTSC，还有SECAM，这是全球现行的三种模拟技术彩色电视的制式
    uint32_t width;//视频宽
    uint32_t height;
    uint32_t aspect_width;//视频宽比例
    uint32_t aspect_height;//视频高比例
} _video_info_t;

typedef struct {
    uint32_t duration;//胞持续时间
    uint32_t vob_id;//vob id
    uint32_t cell_id;//cell id
    uint32_t start;//胞开始地址
    uint32_t end;//胞结束地址
    uint32_t angle;//视角
} _cell_info_t;

typedef struct {
    uint32_t duratio;//章节时间
    uint32_t pgcn;//节目链数量
    uint32_t pgn;//节目数
    uint32_t enter_cell_number;
    uint32_t nr_of_cell;//胞数量
    _cell_info_t *cells;//胞指针
} _chapter_info_t;

typedef struct {
    bool chapter_search_or_play;
    bool exists_in_button_cmd;
    bool exists_in_cell_cmd;
    bool exists_in_prepost_cmd;
    bool exists_in_tt_dom;
    bool multi_or_random_pgc_title;
    bool title_or_time_play;
} _playback_type_t;

typedef struct {
    uint32_t duration;//title对应的视频 时间长度

    _fraction_t fps;//fps
    _video_info_t video_info;//视频信息
    uint32_t nr_of_angles;

    uint32_t nr_of_audios;
    _audio_info_t *audio_infos;

    uint32_t nr_of_subpics;
    _subpic_info_t *subpics;

    uint32_t nr_of_chapters;//章节数量
    _chapter_info_t *chapters;//章节指针

    //非显示用属性
    uint8_t title_set_nr;//vts
    uint8_t vts_ttn;//vts ttn
    bool time_map;

    _playback_type_t pb_ty;
    uint32_t sector;//扇区个数

    int score;//评分
} _title_info_t;

struct titles_info_s {
    uint32_t nr_of_titles;//title总数量
    _title_info_t *titles;//title指针
    bool disney;//迪士尼
};

static const char* get_subp_lang_extension(_subp_lang_extension lang_extension) {
    switch (lang_extension) {
    case reserved_subp:
        return "";
    case not_specified_subp:
        return "not specified";
    case normal_size_subp:
        return "normal size";
    case bigger_size_subp:
        return "bigger size";
    case children_sub:
        return "children";
    case normal_size_closed_subp:
        return "normal size closed";
    case bigger_size_closed_subp:
        return "bigger size closed";
    case children_closed_subp:
        return "children closed";
    case forced_subp:
        return "forced";
    case normal_director_subp:
        return "normal director";
    case bigger_director_subp:
        return "bigger director";
    case director_for_children_sub:
        return "director for children";
    }
    return "";
}

static const char* get_audio_lang_extension(_audio_lang_extension lang_extension) {
    switch (lang_extension) {
    case not_specified_audio:
    case normal_audio:
    case reserved_audio:
        return "";
    case visually_impaired_audio:
        return "visually impaired";
    case director_comments_audio:
        return "director's comments";
    }
    return "";
}

void printf_titles_info(titles_info_t *titles_info, bool detailed) {
    if (titles_info == NULL) {
        return;
    }

    for (uint32_t title_index = 0; title_index < titles_info->nr_of_titles; title_index++) {
        _title_info_t *title_info = &titles_info->titles[title_index];
//        if (title_info->duration < 300) {
//            continue;
//        }
        printf("[Title %02d vts:%02d ttn:%02d %s %d %d/%d/%d/%d %d/%d  ---- %d play:%d]\n",
               title_index+1, title_info->title_set_nr, title_info->vts_ttn,
               sec_to_timestring(title_info->duration),
               title_info->pb_ty.chapter_search_or_play,
               title_info->pb_ty.exists_in_button_cmd,
               title_info->pb_ty.exists_in_cell_cmd,
               title_info->pb_ty.exists_in_prepost_cmd,
               title_info->pb_ty.exists_in_tt_dom,
               title_info->pb_ty.multi_or_random_pgc_title,
               title_info->pb_ty.title_or_time_play,
               title_info->score, title_info->sector);

        if(!detailed) {
            continue;
        }

        printf("\t[video %4s %dx%d (%d:%d) %d/%d fps]\n",
               title_info->video_info.standard, title_info->video_info.width, title_info->video_info.height,
               title_info->video_info.aspect_width, title_info->video_info.aspect_height,
               title_info->fps.num, title_info->fps.den);

        for (uint32_t audio_index = 0; audio_index < title_info->nr_of_audios; audio_index++) {
            _audio_info_t *audio = &title_info->audio_infos[audio_index];
            printf("\t[audio %d %s %s %d %s %s]\n", audio->stream_info.id, audio->stream_info.codec,
                   audio->stream_info.language, audio->sample_rate, audio->quantization,
                   get_audio_lang_extension(audio->lang_extension));
        }

        for (uint32_t subp_index = 0; subp_index < title_info->nr_of_subpics; subp_index++) {
            _subpic_info_t *subp = &title_info->subpics[subp_index];
            printf("\t[subpic %04x %s %s]\n", subp->stream_info.id, subp->stream_info.language,
                   get_subp_lang_extension(subp->lang_extension));
        }

        for (uint32_t chapter_index = 0; chapter_index < title_info->nr_of_chapters; chapter_index++) {
            _chapter_info_t *chapter = &title_info->chapters[chapter_index];
            printf("\t[chapter %02d pgcn %02d pgn %02d %s ]\n", chapter_index+1, chapter->pgcn,
                   chapter->pgn, sec_to_timestring(chapter->duratio));

            for (uint32_t cell_index = 0; cell_index < chapter->nr_of_cell; cell_index++) {
                _cell_info_t *cell = &chapter->cells[cell_index];
                printf("\t\t[cell %02d %s start:%08x end:%08x vob_id:%02d cell_id:%02d angle:%d]\n", cell_index+1,
                       sec_to_timestring(cell->duration), cell->start, cell->end, cell->vob_id, cell->cell_id, cell->angle);
            }
        }
    }
}

static bool init_chapter_info(_title_info_t *title_info, ifo_data_t *ifo_data) {
    if (title_info == NULL || ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return false;
    }

    ifo_handle_t *vts = get_ifo_data(ifo_data, title_info->title_set_nr);
    if (vts == NULL) {
        logger_log(logger, LOGGER_ERR, "can not find vts %d", title_info->title_set_nr);
        return false;
    }

    if (vts->vts_ptt_srpt == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vts_ptt_srpt == NULL");
        return false;
    }

    if (vts->vts_ptt_srpt->nr_of_srpts < title_info->vts_ttn) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vts_ptt_srpt->nr_of_srpts < vts_ttn");
        return false;
    }

    if (vts->vts_ptt_srpt->title == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vts_ptt_srpt->title");
        return false;
    }

    if (vts->vts_pgcit == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vts_pgcit == NULL");
        return false;
    }


    ttu_t *title = &vts->vts_ptt_srpt->title[title_info->vts_ttn-1];
    title_info->nr_of_chapters = title->nr_of_ptts;
    title_info->chapters = (_chapter_info_t*)calloc(title->nr_of_ptts, sizeof(_chapter_info_t));
    if (title_info->chapters == NULL) {
        logger_log(logger, LOGGER_ERR, "alloc memory error %d  (%dx%d)", __LINE__, title->nr_of_ptts, sizeof(_chapter_info_t));
        return false;
    }

    for (uint32_t ptt_index = 0; ptt_index < title->nr_of_ptts; ptt_index++) {
        _chapter_info_t *chapter = &title_info->chapters[ptt_index];
        // 获得chapter信息
        chapter->pgcn = title->ptt[ptt_index].pgcn;
        chapter->pgn = title->ptt[ptt_index].pgn;

        if (chapter->pgcn == 0 || chapter->pgn == 0) {
            continue;
        }

        if (vts->vts_tmapt == NULL
                || vts->vts_tmapt->nr_of_tmaps < chapter->pgcn
                /*|| vts->vts_tmapt->tmap[chapter->pgcn - 1].tmu == 0*/) {
                title_info->time_map = false ;
        }

        //获得每个chapter中cell的信息
        if (vts->vts_pgcit->nr_of_pgci_srp < chapter->pgcn) {
            logger_log(logger, LOGGER_ERR, "ifo file error : number of pgci < pgcn");
            return false;
        }

        pgci_srp_t *pgci_srp = &vts->vts_pgcit->pgci_srp[chapter->pgcn - 1];
        if (pgci_srp->pgc && pgci_srp->pgc->cell_playback && pgci_srp->pgc->cell_position) {

            uint32_t frame = 0;

            if (pgci_srp->pgc->nr_of_programs >= chapter->pgn) {
                uint32_t enter_number = pgci_srp->pgc->program_map[chapter->pgn - 1];
                uint32_t end_number = 0;
                if (chapter->pgn == pgci_srp->pgc->nr_of_programs) {
                    end_number = pgci_srp->pgc->nr_of_cells + 1;
                }
                else {
                    end_number = pgci_srp->pgc->program_map[chapter->pgn];
                }

                if (end_number <= enter_number) {
                    continue;
                }
                chapter->nr_of_cell = end_number - enter_number;
                chapter->cells = (_cell_info_t*)calloc(chapter->nr_of_cell, sizeof(_cell_info_t));
                if (chapter->cells == NULL) {
                    logger_log(logger, LOGGER_ERR, "alloc memory error %d (%d*%d)", __LINE__, chapter->nr_of_cell, sizeof(_cell_info_t));
                    return false;
                }
                enter_number--;
                uint32_t angle_index = 0;
                for (uint32_t cell_index = 0; cell_index < chapter->nr_of_cell; cell_index++) {
                    chapter->cells[cell_index].duration = playbacktimetosec(&pgci_srp->pgc->cell_playback[cell_index + enter_number].playback_time);
                    chapter->cells[cell_index].start = pgci_srp->pgc->cell_playback[cell_index + enter_number].first_sector;
                    chapter->cells[cell_index].end = pgci_srp->pgc->cell_playback[cell_index + enter_number].last_sector;
                    frame += playbacktimetoframe(&pgci_srp->pgc->cell_playback[cell_index + enter_number].playback_time);

                    chapter->cells[cell_index].vob_id = pgci_srp->pgc->cell_position[cell_index + enter_number].vob_id_nr;
                    chapter->cells[cell_index].cell_id = pgci_srp->pgc->cell_position[cell_index + enter_number].cell_nr;

                    if (pgci_srp->pgc->cell_playback[cell_index + enter_number].block_type == 0x00) {
                        angle_index = 0;
                        chapter->cells[cell_index].angle = 0;
                    }
                    else if (pgci_srp->pgc->cell_playback[cell_index + enter_number].block_mode == 0x01) {
                        angle_index = 0;
                    }
                    else {
                        angle_index++;
                    }
                    chapter->cells[cell_index].angle = angle_index;
                }
            }

            uint32_t framerate = get_framerate(&pgci_srp->pgc->playback_time);
            if (framerate > 0) {
                chapter->duratio = frame / framerate;
                title_info->duration += chapter->duratio;
                title_info->fps.num = framerate;
                title_info->fps.den = 100;
            }
        }
        else {
            logger_log(logger, LOGGER_INFO, "cell_playback == NULL or cell_position == NULL");
        }

    }

    if (title_info->vts_ttn == 0) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vts_ttn == 0");
        return false;
    }

    return true;
}

static bool init_video_info(_title_info_t *title_info, ifo_data_t *ifo_data) {
    if (title_info == NULL || ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return false;
    }

    ifo_handle_t *vts = get_ifo_data(ifo_data, title_info->title_set_nr);
    if (vts == NULL) {
        logger_log(logger, LOGGER_ERR, "can not find vts %d", title_info->title_set_nr);
        return false;
    }

    if (vts->vtsi_mat == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vtsi_mat == NULL");
        return false;
    }

    video_attr_t *video_attr = &vts->vtsi_mat->vts_video_attr;
    switch (video_attr->mpeg_version) {
    case 0:
        strncpy(title_info->video_info.stream_info.codec, "mpeg1", MAX_CODEC_LEN);
        break;
    case 1:
        strncpy(title_info->video_info.stream_info.codec, "mpeg2", MAX_CODEC_LEN);
        break;
    default:
        logger_log(logger, LOGGER_ERR, "unknow mpeg version");
        break;
    }

    switch (video_attr->video_format) {
    case 0:
        strncpy(title_info->video_info.standard, "ntsc", MAX_STANDARD_LEN);
        break;
    case 1:
        strncpy(title_info->video_info.standard, "pal", MAX_STANDARD_LEN);
        break;
    default:
        logger_log(logger, LOGGER_ERR, "unknow video format");
        break;
    }

    switch (video_attr->display_aspect_ratio) {
    case 0:
        title_info->video_info.aspect_width = 4;
        title_info->video_info.aspect_height = 3;
        break;
    case 3:
        title_info->video_info.aspect_width = 16;
        title_info->video_info.aspect_height = 9;
        break;
    default:
        logger_log(logger, LOGGER_ERR, "unknow display aspect ratio");
        break;
    }

    if (video_attr->video_format != 0) {
        title_info->video_info.height = 576;
    }
    else {
        title_info->video_info.height = 480;
    }

    switch (video_attr->picture_size) {
    case 0:
    case 1:
        title_info->video_info.width = 720;
        break;
    case 3:
        title_info->video_info.height /= 2;
    case 2:
        title_info->video_info.width = 352;
        break;
    default:
        logger_log(logger, LOGGER_ERR, "unknow picture size");
        break;
    }
    return true;
}


static bool init_audio_info(_title_info_t *title_info, ifo_data_t *ifo_data) {
    if (title_info == NULL || ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return false;
    }

    ifo_handle_t *vts = get_ifo_data(ifo_data, title_info->title_set_nr);
    if (vts == NULL) {
        logger_log(logger, LOGGER_ERR, "can not find vts %d", title_info->title_set_nr);
        return false;
    }

    if (vts->vtsi_mat == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vtsi_mat == NULL");
        return false;
    }

    if (vts->vtsi_mat->nr_of_vts_audio_streams > 8) {
        logger_log(logger, LOGGER_ERR, "ifo file error : nr_of_vts_audio_streams > 8");
        return false;
    }

    if (vts->vts_ptt_srpt == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file err : vts_ptt_srpt == NULL");
        return false;
    }

    if (vts->vts_ptt_srpt->nr_of_srpts < title_info->vts_ttn) {
        logger_log(logger, LOGGER_ERR, "ifo file err : vts_ptt_srpt->nr_of_srpts < vts_ttn");
        return false;
    }

    ttu_t *title = &vts->vts_ptt_srpt->title[title_info->vts_ttn-1];

    if (vts->vts_pgcit == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file err : vts_pgcit == NULL");
        return false;
    }

    // 简化处理，使用第一个pgcn获取pgc指针
    if (vts->vts_pgcit->nr_of_pgci_srp < title->ptt[0].pgcn) {
        logger_log(logger, LOGGER_ERR, "ifo file err : nr_of_pgci_srp < pgcn");
        return false;
    }

    pgci_srp_t *pgci_srp = &vts->vts_pgcit->pgci_srp[title->ptt[0].pgcn-1];
    if (pgci_srp->pgc == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file err : pgc == NULL");
        return false;
    }


    uint32_t audio_count = 0;
    for (uint32_t audio_index = 0; audio_index < vts->vtsi_mat->nr_of_vts_audio_streams; audio_index++) {
        if (pgci_srp->pgc->audio_control[audio_index] & 0x8000) {
            audio_count++;
        }
    }
    if (audio_count == 0) {
        return true;
    }

    title_info->nr_of_audios = audio_count;
    title_info->audio_infos = (_audio_info_t*)calloc(audio_count, sizeof(_audio_info_t));
    if (title_info->audio_infos == NULL) {
        logger_log(logger, LOGGER_ERR, "alloc memory error %d", __LINE__);
        return false;
    }

    uint32_t real_audio_index = 0;
    for (uint32_t audio_index = 0; audio_index < vts->vtsi_mat->nr_of_vts_audio_streams; audio_index++) {

        if (pgci_srp->pgc->audio_control[audio_index] & 0x8000) {
        }
        else {
            logger_log(logger, LOGGER_INFO, "audio control %04x", pgci_srp->pgc->audio_control[audio_index]);
            continue;
        }

        _audio_info_t *audio_info = &title_info->audio_infos[real_audio_index++];
        audio_attr_t *vts_audio_attr = &vts->vtsi_mat->vts_audio_attr[audio_index];

        int start = 0;
        switch (vts_audio_attr->audio_format) {
        case 0:
            strncpy(audio_info->stream_info.codec, "ac3", MAX_CODEC_LEN);
            start = 0x80;
            break;
        case 2:
            strncpy(audio_info->stream_info.codec, "mpeg1", MAX_CODEC_LEN);
            start = 0;
            break;
        case 3:
            strncpy(audio_info->stream_info.codec, "mpeg2", MAX_CODEC_LEN);
            start = 0;
            break;
        case 4:
            strncpy(audio_info->stream_info.codec, "lpcm", MAX_CODEC_LEN);
            start = 0xa0;
            break;
        case 6:
            strncpy(audio_info->stream_info.codec, "dts", MAX_CODEC_LEN);
            start = 0x88;
            break;
        default:
            logger_log(logger, LOGGER_ERR, "unknow audio format");

        }

        audio_info->stream_info.id = ((pgci_srp->pgc->audio_control[audio_index] >> 8) & 0x07) + start;

        switch (vts_audio_attr->lang_type) {
        case 0:
            break;
        case 1:
            snprintf(audio_info->stream_info.language, MAX_LANGUAGE_LEN,
                     "%c%c", vts_audio_attr->lang_code>>8, vts_audio_attr->lang_code & 0xff);
            break;
        default:
            logger_log(logger, LOGGER_ERR, "unknow language type");
        }

        switch (vts_audio_attr->quantization) {
        case 0:
            strcpy(audio_info->quantization, "16bit");
            break;
        case 1:
            strcpy(audio_info->quantization, "20bit");
            break;
        case 2:
            strcpy(audio_info->quantization, "24bit");
            break;
        case 3:
            strcpy(audio_info->quantization, "drc");
            break;
        default:
            logger_log(logger, LOGGER_ERR, "unknow audio quantization");
            break;
        }

        if (vts_audio_attr->sample_frequency == 0) {
            audio_info->sample_rate = 48000;
        }
        else {
            logger_log(logger, LOGGER_ERR, "unknow sample frequency");
        }

        audio_info->nr_of_channels = vts_audio_attr->channels + 1;

        switch (vts_audio_attr->lang_extension) {
        case 0:
        case 1:
            audio_info->lang_extension = not_specified_audio;
            break;
        case 2:
            audio_info->lang_extension = normal_audio;
            break;
        case 3:
            audio_info->lang_extension = visually_impaired_audio;
            break;
        case 4:
        case 5:
            audio_info->lang_extension = director_comments_audio;
            break;
        default:
            logger_log(logger, LOGGER_ERR, "unknow lang extension");
            break;
        }
    }
    return true;
}

static bool init_subpicture_info(_title_info_t *title_info, ifo_data_t *ifo_data) {
    if (title_info == NULL || ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return false;
    }

    ifo_handle_t *vts = get_ifo_data(ifo_data, title_info->title_set_nr);
    if (vts == NULL) {
        logger_log(logger, LOGGER_ERR, "can not find vts %d", title_info->title_set_nr);
        return false;
    }

    if (vts->vtsi_mat == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : vtsi_mat == NULL");
        return false;
    }

    if (vts->vtsi_mat->nr_of_vts_subp_streams > 32) {
        logger_log(logger, LOGGER_ERR, "ifo file error : nr_of_vts_subp_streams > 32");
        return false;
    }

    if (vts->vts_ptt_srpt == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file err : vts_ptt_srpt == NULL");
        return false;
    }

    if (vts->vts_ptt_srpt->nr_of_srpts < title_info->vts_ttn) {
        logger_log(logger, LOGGER_ERR, "ifo file err : vts_ptt_srpt->nr_of_srpts < vts_ttn");
        return false;
    }

    ttu_t *title = &vts->vts_ptt_srpt->title[title_info->vts_ttn-1];

    if (vts->vts_pgcit == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file err : vts_pgcit == NULL");
        return false;
    }

    // 简化处理，使用第一个pgcn获取pgc指针
    if (vts->vts_pgcit->nr_of_pgci_srp < title->ptt[0].pgcn) {
        logger_log(logger, LOGGER_ERR, "ifo file err : nr_of_pgci_srp < pgcn");
        return false;
    }

    pgci_srp_t *pgci_srp = &vts->vts_pgcit->pgci_srp[title->ptt[0].pgcn-1];
    if (pgci_srp->pgc == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file err : pgc == NULL");
        return false;
    }

    uint32_t subpic_count = 0;
    for (uint32_t subpic_index = 0; subpic_index < vts->vtsi_mat->nr_of_vts_subp_streams; subpic_index++) {
        if (pgci_srp->pgc->subp_control[subpic_index] & 0x80000000) {
            subpic_count++;
        }
    }
    if (subpic_count == 0) {
        return true;
    }

    title_info->nr_of_subpics = subpic_count;
    title_info->subpics = (_subpic_info_t*)calloc(subpic_count, sizeof(_subpic_info_t));
    if (title_info->subpics == NULL) {
        logger_log(logger, LOGGER_ERR, "alloc memory error %d", __LINE__);
        return false;
    }

    uint32_t real_subpic_index = 0;
    for (uint32_t subpic_index = 0; subpic_index < title_info->nr_of_subpics; subpic_index++) {
        subp_attr_t *vts_subp_attr = &vts->vtsi_mat->vts_subp_attr[subpic_index];

        if (pgci_srp->pgc->subp_control[subpic_index] & 0x80000000) {
        }
        else {
            continue;
        }

        _subpic_info_t *subpic = &title_info->subpics[real_subpic_index++];
        subpic->stream_info.id = (pgci_srp->pgc->subp_control[subpic_index] >> 8) & 0x1f1f;

        if (isalpha(vts_subp_attr->lang_code >> 8) && isalpha(vts_subp_attr->lang_code & 0xff)) {
            snprintf(subpic->stream_info.language, MAX_LANGUAGE_LEN,
                     "%c%c", vts_subp_attr->lang_code >> 8, vts_subp_attr->lang_code & 0xff);
        }

        switch (vts_subp_attr->lang_extension) {
        case 0:
            subpic->lang_extension = not_specified_subp;
            break;
        case 1:
            subpic->lang_extension = normal_size_subp;
            break;
        case 2:
            subpic->lang_extension = bigger_size_subp;
            break;
        case 3:
            subpic->lang_extension = children_sub;
            break;
        case 4:
        case 8:
        case 10:
        case 11:
        case 12:
            subpic->lang_extension = reserved_subp;
            break;
        case 5:
            subpic->lang_extension = normal_size_closed_subp;
            break;
        case 6:
            subpic->lang_extension = bigger_size_closed_subp;
            break;
        case 7:
            subpic->lang_extension = children_closed_subp;
            break;
        case 9:
            subpic->lang_extension = forced_subp;
            break;
        case 13:
            subpic->lang_extension = normal_director_subp;
            break;
        case 14:
            subpic->lang_extension = bigger_director_subp;
            break;
        case 15:
            subpic->lang_extension = director_for_children_sub;
            break;
        default:
            logger_log(logger, LOGGER_ERR, "");

        }
    }

    return true;
}

titles_info_t* init_titles_info(ifo_data_t *ifo_data) {
    if (ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s parameter error", __func__);
        return NULL;
    }

    ifo_handle_t *video_ts = get_ifo_data(ifo_data, 0);
    if (video_ts == NULL) {
        logger_log(logger, LOGGER_ERR, "can not get video_ts handle");
        return NULL;
    }

    if (video_ts->tt_srpt == NULL) {
        logger_log(logger, LOGGER_ERR, "ifo file error : tt_srpt == NULL");
        return NULL;
    }

    titles_info_t *titles_info = (titles_info_t*)calloc(1, sizeof(titles_info_t));
    if (titles_info == NULL) {
        logger_log(logger, LOGGER_ERR, "alloc memory error %d", __LINE__);
        return NULL;
    }

    //获取title总数量
    titles_info->nr_of_titles = video_ts->tt_srpt->nr_of_srpts;
    titles_info->titles = (_title_info_t*)calloc(titles_info->nr_of_titles, sizeof(_title_info_t));
    for (uint32_t title_index = 0; title_index < titles_info->nr_of_titles; title_index++) {
        _title_info_t *title_info = &titles_info->titles[title_index];
        title_info_t *title = &video_ts->tt_srpt->title[title_index];

        title_info->title_set_nr = title->title_set_nr;
        title_info->vts_ttn = title->vts_ttn;
        title_info->nr_of_angles = title->nr_of_angles;
        title_info->pb_ty.chapter_search_or_play = title->pb_ty.chapter_search_or_play;
        title_info->pb_ty.exists_in_button_cmd = title->pb_ty.jlc_exists_in_button_cmd;
        title_info->pb_ty.exists_in_cell_cmd = title->pb_ty.jlc_exists_in_cell_cmd;
        title_info->pb_ty.exists_in_prepost_cmd = title->pb_ty.jlc_exists_in_prepost_cmd;
        title_info->pb_ty.exists_in_tt_dom = title->pb_ty.jlc_exists_in_tt_dom;
        title_info->pb_ty.multi_or_random_pgc_title = title->pb_ty.multi_or_random_pgc_title;
        title_info->pb_ty.title_or_time_play = title->pb_ty.title_or_time_play;

        title_info->time_map = true;

        if (!init_video_info(title_info, ifo_data)) {

        }

        if (!init_audio_info(title_info, ifo_data)) {

        }

        if (!init_subpicture_info(title_info, ifo_data)) {

        }

        if (!init_chapter_info(title_info, ifo_data)) {
            if (title_info->chapters) {
                free(title_info->chapters);
                title_info->chapters = NULL;
                title_info->nr_of_chapters = 0;
            }
        }
    }

    return titles_info;
}

void destroy_titles_info(titles_info_t *titles_info) {
    if (titles_info) {
        if (titles_info->titles) {
            for (uint32_t title_index = 0; title_index < titles_info->nr_of_titles; title_index++) {
                _title_info_t *title_info = &titles_info->titles[title_index];
                if (title_info->audio_infos) {
                    free(title_info->audio_infos);
                    title_info->audio_infos = NULL;
                    title_info->nr_of_audios = 0;
                }
                if (title_info->subpics) {
                    free(title_info->subpics);
                    title_info->subpics = NULL;
                    title_info->nr_of_subpics = 0;
                }
                if (title_info->chapters) {
                    free(title_info->chapters);
                    title_info->chapters = NULL;
                    title_info->nr_of_chapters = 0;
                }
            }
            free(titles_info->titles);
            titles_info->titles = NULL;
            titles_info->nr_of_titles = 0;
        }

        free(titles_info);
        titles_info = NULL;
    }
}


static void tmap_filter(_title_info_t *title) {
    // 绝对的判断标准，不让它有翻身的机会。
    title->score += title->time_map ? 1000 : -1000;

#ifdef QT_QML_DEBUG
    logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d tmap filter %d", title->title_set_nr, title->vts_ttn, title->time_map ? 1000 : -1000);
#endif
}

static int cell_match(void *a, void *b) {
    if (a == NULL || b  == NULL) {
        return 0;
    }

    if (a == b) {
        return 1;
    }

    _cell_info_t *cell_a = (_cell_info_t*)a;
    _cell_info_t *cell_b = (_cell_info_t*)b;

    if (cell_a->start == cell_b->start && cell_a->end == cell_b->end) {
        return 1;
    }
    return 0;
}

static void vob_cell_id_filter(_title_info_t *title) {
    uint32_t vob_id = 0, cell_id = 0;

    if (title->chapters == NULL && title->nr_of_chapters == 0) {
        logger_log(logger, LOGGER_INFO, "vts %02d ttn %02d number of chapters = 0", title->title_set_nr, title->vts_ttn);
        title->score -= 20;
#ifdef QT_QML_DEBUG
        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d no chapter -20", title->title_set_nr, title->vts_ttn);
#endif
        return;
    }

    list_t *list = list_new();
    list->match = cell_match;
    for (uint32_t chapter_index = 0; chapter_index < title->nr_of_chapters; chapter_index++) {
        _chapter_info_t *chapter = &title->chapters[chapter_index];
        for (uint32_t cell_index = 0; cell_index < chapter->nr_of_cell; cell_index++) {
            if (chapter->cells[cell_index].angle != 0) {
                //只分析一个角度
                continue;
            }
            list_rpush(list, list_node_new(&chapter->cells[cell_index]));
        }
    }

    // 掐头
    list_node_t *node = NULL;
    while (true) {
        node = list_at(list, 0);
        if (node == NULL) {
            break;
        }

        _cell_info_t *cell = (_cell_info_t*)node->val;
        if (cell->duration <= 1) {
            list_remove(list, node);
        }
        else {
            break;
        }
    }

    // 去尾
    while(true) {
        node = list_at(list, list->len-1);
        if (node == NULL) {
            break;
        }

        _cell_info_t *cell = (_cell_info_t*)node->val;
        if (cell->duration <= 1) {
            list_remove(list, node);
        }
        else {
            break;
        }
    }

    // the_hunger_game.iso
    // 检查中间是否有长度是0的cell
    int zero_cell_count = 0;
    uint32_t cell_duration = 0;
    for (uint32_t i=0; i<list->len; i++) {
        node = list_at(list, i);
        if (node == NULL) {
            break;
        }

        _cell_info_t *cell = (_cell_info_t*)node->val;
        if (cell->duration == 0) {
            // 至少10秒后的0cell才统计。
            if (cell_duration > 10) {
                zero_cell_count++;
                break;
            }
        }
        else {
            cell_duration += cell->duration;
        }
    }

    //BIG_BANG_THEORY_SEASON_5_DISC3.iso
    //主电影里就是有三个0的cell
    //从减1000改为减500
    if (zero_cell_count > 0) {
        title->score -= 500;
#ifdef QT_QML_DEBUG
        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d cell 00:00:00 -1000", title->title_set_nr, title->vts_ttn);
#endif
    }

    node = list_lpop(list);
    while(node) {
        _cell_info_t *cell = (_cell_info_t*)node->val;
        if (cell->vob_id > vob_id) {
            if (cell->cell_id == 1) {
                title->score += 20;
#ifdef QT_QML_DEBUG
                logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d cell start +20", title->title_set_nr, title->vts_ttn);
#endif
            }
        }
        else if (cell->vob_id == vob_id) {
            if (cell->cell_id != cell_id + 1) {
                title->score -= 10;
#ifdef QT_QML_DEBUG
                logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d cell uncontinuous -10", title->title_set_nr, title->vts_ttn);
#endif
            }
            else {

            }
        }
        else {
            //vob 后退
            title->score -= 50;
        }

        list_node_t *tmp = list_find(list, cell);
        if (tmp != NULL) {
            //有重复的cell
            title->score -= 100;
        }

        vob_id = cell->vob_id;
        cell_id = cell->cell_id;

        node = list_lpop(list);
    }

    list_destroy(list);
}

static void audio_filter(_title_info_t *title) {
    for (uint32_t audio_index = 0; audio_index < title->nr_of_audios; audio_index++) {
        // 有一个音轨
        title->score += 10;
#ifdef QT_QML_DEBUG
        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d audio 10", title->title_set_nr, title->vts_ttn);
#endif

        _audio_info_t *audio = &title->audio_infos[audio_index];
        if (strlen(audio->stream_info.language) > 0) {
            // 音轨指定了语言
            title->score += 20;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d audio language 20", title->title_set_nr, title->vts_ttn);
#endif
        }

        // 指定了语言的特殊属性
        switch (audio->lang_extension) {
        case reserved_audio:
        case not_specified_audio:
            break;
        case normal_audio:
            title->score += 10;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d audio language ext normal 10", title->title_set_nr, title->vts_ttn);
#endif
            break;
        case visually_impaired_audio:
        case director_comments_audio:
            title->score += 20;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d audio language ext visually or director 20", title->title_set_nr, title->vts_ttn);
#endif
            break;
        }

        // 六声道
        if (audio->nr_of_channels == 6) {
            title->score += 15;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d audio 6 channels 15", title->title_set_nr, title->vts_ttn);
#endif
        }

        // 高保真编码
        if (strcmp(audio->stream_info.codec, "dts") == 0
                && strcmp(audio->stream_info.codec, "lpcm") == 0) {
            title->score += 20;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d audio 6 channels 20", title->title_set_nr, title->vts_ttn);
#endif
        }
    }
}

static void subp_filter(_title_info_t *title) {
    for (uint32_t subp_index = 0; subp_index < title->nr_of_subpics; subp_index++) {
        _subpic_info_t *subp = &title->subpics[subp_index];
        //有一个字幕就加10分
        title->score += 20;
#ifdef QT_QML_DEBUG
        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d subp 20", title->title_set_nr, title->vts_ttn);
#endif


        //指定了语言
        if (strlen(subp->stream_info.language) > 0) {
            title->score += 30;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d subp language 30", title->title_set_nr, title->vts_ttn);
#endif
        }

        switch (subp->lang_extension) {
        case reserved_subp:
        case not_specified_subp:
            break;
        case normal_size_subp:
        case bigger_size_subp:
        case children_sub:
        case normal_size_closed_subp:
        case bigger_size_closed_subp:
        case children_closed_subp:
        case forced_subp:
        case normal_director_subp:
        case bigger_director_subp:
        case director_for_children_sub:
            title->score += 10;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d subp language ext 10", title->title_set_nr, title->vts_ttn);
#endif
            break;
        default:
            break;
        }
    }
}

static void pb_type_filter(_title_info_t *title) {
    if (title->pb_ty.multi_or_random_pgc_title) {
        //这个也不能让他翻身
        title->score -= 1000;
#ifdef QT_QML_DEBUG
        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d random pgc -1000", title->title_set_nr, title->vts_ttn);
#endif
    }
}

static void chapter_duration_filter(_title_info_t *title) {
    // 章节长度是0的，每多一个就比上次多扣10分。
    uint32_t count = 1;
    for (uint32_t chapter_index = 0; chapter_index < title->nr_of_chapters; chapter_index++) {
        _chapter_info_t *chapter = &title->chapters[chapter_index];
        if (chapter->duratio < 30) {
            title->score -= 10 /* count*/;
#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d chapter duration -%d", title->title_set_nr, title->vts_ttn, 10 * count);
#endif
            count++;
        }
    }
}

static void title_duration_filter(_title_info_t *title) {
    if (title->duration > 3600) {
        title->score += 300;
    }
    else if (title->duration > 2400) {
        title->score += 150;
    }
    else if (title->duration > 1800) {
        title->score += 100;
    }
    else if (title->duration < 300){
        title->score -= 300;
    }

    if (title->nr_of_chapters > 60) {
        title->score -= 50;
    }
    if (title->nr_of_chapters > 100) {
        title->score -= 200;
    }
    if (title->nr_of_chapters > 150) {
        title->score -= 400;
    }
    if (title->nr_of_chapters > 200) {
        title->score -= 800;
    }
}

typedef void (*_title_filter_func)(_title_info_t *title);

_title_filter_func title_filter[] = {
    pb_type_filter,
    tmap_filter,
    vob_cell_id_filter,
    audio_filter,
    subp_filter,
    chapter_duration_filter,
    title_duration_filter,
    NULL
};

static void titles_common_score(titles_info_t *titles_info, ifo_data_t *ifo_data) {
    (void)ifo_data;

    if (titles_info == NULL) {
        return ;
    }

    uint32_t i = 0;
    while (1) {
        _title_filter_func func = title_filter[i++];
        if (func == NULL) {
            break;
        }

        for (uint32_t title_index = 0; title_index < titles_info->nr_of_titles; title_index++) {
            func(&titles_info->titles[title_index]);
        }
    }
}

typedef struct {
    bool cell_continuous;
    bool come_back;
    uint32_t jump_sector;
    uint32_t play_sector;
} _title_cells_sector_score_t;


static void titles_cell_sector_score(titles_info_t *titles_info, ifo_data_t *ifo_data) {
    (void)ifo_data;

    _title_cells_sector_score_t *title_cells_sector_score = (_title_cells_sector_score_t*)calloc(titles_info->nr_of_titles, sizeof(_title_cells_sector_score_t));
    if (title_cells_sector_score == NULL) {
        return;
    }

    for (uint32_t title_index = 0; title_index < titles_info->nr_of_titles; title_index++) {
        _title_info_t *title = &titles_info->titles[title_index];
        _title_cells_sector_score_t *cell_score = &title_cells_sector_score[title_index];
        list_t *list = list_new();

        //提取当前title中所有cell
        for (uint32_t chapter_index = 0; chapter_index < title->nr_of_chapters; chapter_index++) {
            _chapter_info_t *chapter = &title->chapters[chapter_index];
            for (uint32_t cell_index = 0; cell_index < chapter->nr_of_cell; cell_index++) {
                if (chapter->cells[cell_index].angle != 0) {
                    continue;
                }
                list_rpush(list, list_node_new(&chapter->cells[cell_index]));
            }
        }

        list_t *processed = list_new();
        processed->match = cell_match;
        _cell_info_t *last_cell = NULL;
        while (1) {
            list_node_t *node = list_lpop(list);
            if (node == NULL) {
                break;
            }

            _cell_info_t *cell = (_cell_info_t*)node->val;
            if (cell == NULL) {
                continue;
            }

            cell_score->play_sector += cell->end - cell->start + 1;

            if (last_cell != NULL) {
                if (last_cell->end > cell->start) {
                    // 往回跳转
                    // 只能跳转一次，且不能是之前出现过的cell
                    // 既，每个cell在正常的title中只能出现一次
                    if (list_find(processed, cell) != NULL) {
                        // 是分析过的cell
                        title->score -= 20;
#ifdef QT_QML_DEBUG
                        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d cell find -20", title->title_set_nr, title->vts_ttn);
#endif
                    }

                    if (!cell_score->come_back) {
                        cell_score->come_back = true;
                    }
                    else {
                        // 出现回跳情况，见一次减10分
                        title->score -= 10;
#ifdef QT_QML_DEBUG
                        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d end: %08x start: %08x cell come back -10",
                                   title->title_set_nr, title->vts_ttn, last_cell->end, cell->start);
#endif
                    }
                }
                else if (last_cell->end == cell->start) {
                    //不可能出现的情况，出现就减100
                    title->score -= 100;
#ifdef QT_QML_DEBUG
                    logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d cell end(%d) = start(%d) -100", title->title_set_nr, title->vts_ttn,
                               last_cell->end, cell->start);
#endif
                }
                else {
                    cell_score->jump_sector += cell->start - last_cell->end - 1;
                }
            }

            last_cell = cell;
            list_rpush(processed, list_node_new(cell));
        }

        list_destroy(list);
        list_destroy(processed);

        // 播放占比越大分数越高
        if (cell_score->jump_sector == 0 && cell_score->play_sector == 0) {
            title->score -= 100;
        }
        else {
            title->score += 100.0 * ((double)cell_score->play_sector / (double)(cell_score->play_sector + cell_score->jump_sector));
        }
#ifdef QT_QML_DEBUG
        logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d cell jump play %d", title->title_set_nr, title->vts_ttn,
                   (int)(100.0 * ((double)cell_score->play_sector / (double)(cell_score->play_sector + cell_score->jump_sector))));
#endif

        int score = 0;
        if (title->duration > 0) {
            double cell_average = (double)cell_score->play_sector / (double)title->duration;
            if (cell_average > 400) {
                score += 20;
            }
            else if (cell_average > 370) {
                score += 50;
            }
            else if (cell_average > 350) {
                score += 80;
            }
            else if (cell_average > 300) {
                score += 50;
            }
            else if (cell_average > 200) {
                score += 20;
            }
            else if (cell_average < 50) {
                score -= 1000;
            }
            else if (cell_average < 100) {
                score -= 50;
            }

            title->score += score;

#ifdef QT_QML_DEBUG
            logger_log(logger, LOGGER_DEBUG, "vts : %02d ttn : %02d average cell %0.3f (%d)", title->title_set_nr, title->vts_ttn, cell_average, score);
#endif
        }

        title->sector = cell_score->play_sector;
        if (title->sector < 200000) {
            title->score -= 500;
        }
        else if (title->sector < 300000) {
            title->score -= 300;
        }
        else if (title->sector < 500000) {
            title->score -= 200;
        }
        else if (title->sector > 1000000) {
            title->score += 100;
        }
        else if (title->sector > 2000000) {
            title->score += 200;
        }
        else if (title->sector > 1000000) {
            title->score += 300;
        }
    }
    free(title_cells_sector_score);
}


void p90xx_titles_filter_func(titles_info_t *titles, ifo_data_t *ifo_data) {
    if (titles == NULL || ifo_data == NULL) {
        return;
    }
    //不是P90x系列的DVD
    const char * volume = get_volume(ifo_data);
    if (volume == NULL) {
        return;
    }

    if (strncasecmp(volume, "<P90x", 5)) {
        return;
    }

    //是P90x系列的DVD
    if (titles->nr_of_titles <= 4) {
        return;
    }

    //找到最大的两个评分
    int first = 0, second = 0;
    for (uint32_t i=0; i<titles->nr_of_titles; i++) {
        if (titles->titles[i].score > second) {
            if (titles->titles[i].score > first) {
                second = first;
                first = titles->titles[i].score;
            }
            else if (titles->titles[i].score < first) {
                second = titles->titles[i].score;
            }

            printf("first:%d     second:%d\n", first, second);
        }
    }

    if (second == 0) {
        return;
    }

    //统计两类title的数量和时间长度
    //uint32_t first_duration = 0, second_duration = 0;
    int first_count = 0, second_count = 0;
    uint32_t *first_array = NULL, *second_array = NULL;
    first_array = (uint32_t*)calloc(titles->nr_of_titles, sizeof(uint32_t));
    second_array = (uint32_t*)calloc(titles->nr_of_titles, sizeof(uint32_t));

    if (first_array && second_array) {
        for (uint32_t i=0; i<titles->nr_of_titles; i++) {
            if (titles->titles[i].score == first) {
                first_array[first_count++] = i;
            }
            else if (titles->titles[i].score == second) {
                second_array[second_count++] = i;
            }
        }
    }

    for (int i=1; i<first_count; i++) {
        titles->titles[first_array[i]].score = second;
    }

    if (second_count > 0) {
        titles->titles[second_array[0]].score = first;
    }

    free(first_array);
    free(second_array);
}

typedef void (*_titles_filter_func)(titles_info_t *titles, ifo_data_t *ifo_data);


_titles_filter_func titles_filter[] = {
    titles_common_score,
    titles_cell_sector_score,

    //这个要在最后
    p90xx_titles_filter_func,
    NULL
};


void title_score(titles_info_t *titles_info, ifo_data_t *ifo_data) {
    if (titles_info == NULL) {
        return;
    }

    uint32_t i = 0;
    while (1) {
        _titles_filter_func func = titles_filter[i++];
        if (func == NULL) {
            break;
        }

        func(titles_info, ifo_data);
    }
#ifdef QT_QML_DEBUG
    printf_titles_info(titles_info, true);
#endif
}


uint32_t *get_main_title(titles_info_t *titles_info, int *count) {
    if (titles_info == NULL || count == NULL) {
        return NULL;
    }

    int max_score = 0;
    *count = 0;
    //找最高分数的title， 并统计个数
    for (uint32_t i = 0; i < titles_info->nr_of_titles; i++) {
        if (titles_info->titles[i].duration < 300) {
            continue;
        }
        if (titles_info->titles[i].score > max_score) {
            max_score = titles_info->titles[i].score;
            *count = 1;
        }
        else if (titles_info->titles[i].score == max_score) {
            (*count)++;
        }
    }

    if (*count == 0) {
        //没有高分title，所有title评分都小于0.
        //直接返回时间最长的title
        uint32_t duration = 0;
        int max_title = 0;
        uint32_t *titles = (uint32_t*)calloc(1, sizeof(uint32_t));
        *count = 1;
        for (uint32_t i = 0; i < titles_info->nr_of_titles; i++) {
            if (titles_info->titles[i].duration > duration) {
                max_title = i+1;
                duration = titles_info->titles[i].duration;
            }
        }

        if (duration > 0 && max_title > 0) {
            titles[0] = max_title;
        }
        else {
            titles[0] = 1;
        }
        return titles;
    }


    if (*count > 1) {
        if (titles_info->disney) {
            //是Disney，从评分最高的title中，找能播放的sector最多的。
            uint32_t max_paly_sector_title = 0;
            uint32_t max_play_sector = 0;

            for (uint32_t i = 0; i < titles_info->nr_of_titles; i++) {
                if (titles_info->titles[i].duration < 300) {
                    continue;
                }

                if (titles_info->titles[i].score == max_score) {
                    if (max_play_sector < titles_info->titles[i].sector) {
                        max_play_sector = titles_info->titles[i].sector;
                        max_paly_sector_title = i + 1;
                    }
                }
            }

            *count = 1;
            uint32_t *titles = (uint32_t*)calloc(*count, sizeof(uint32_t));
            titles[0] = max_paly_sector_title;
            return titles;
        }
        else {
            // 非disney，直接返回结果
            uint32_t *titles = (uint32_t*)calloc(*count, sizeof(uint32_t));
            if (titles == NULL) {
                return 0;
            }

            *count = 0;
            for (uint32_t i = 0; i < titles_info->nr_of_titles; i++) {
                if (titles_info->titles[i].duration < 300) {
                    continue;
                }
                if (titles_info->titles[i].score == max_score
                        /*&& titles_info->titles[i].sector == max_play_sector*/) {
                    titles[(*count)++] = i+1;
                }
            }

            return titles;
        }
    }
    else {
        uint32_t *titles = (uint32_t*)calloc(*count, sizeof(uint32_t));
        if (titles == NULL) {
            return 0;
        }

        *count = 0;
        for (uint32_t i = 0; i < titles_info->nr_of_titles; i++) {
            if (titles_info->titles[i].duration < 300) {
                continue;
            }
            if (titles_info->titles[i].score == max_score) {
                titles[(*count)++] = i+1;
                break;
            }
        }

        return titles;

    }
    *count = 0;
    return NULL;
}












