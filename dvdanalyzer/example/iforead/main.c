#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "digiarty_dvd_info.h"

void print_dvd_info(digiarty_dvd_info* dvd_info) {
    if (dvd_info) {
        uint32_t i;
        printf("[");
        for(i=0; i<dvd_info->number_of_title; i++) {
            if(i != 0)
            {
                printf(",");
            }
            printf("{");
            float fps = 0.0;
            if (dvd_info->title_info[i].playback_time.fps == fps_25) {
                fps = 25;
            }
            else if (dvd_info->title_info[i].playback_time.fps == fps_2997) {
                fps = 29.97;
            }

            char *main_title = "false";
            if (dvd_info->title_info[i].is_main_title == 1) {
                main_title = "true";
            }
            char *enable_check = "false";
            if (dvd_info->title_info[i].is_enable_title == 1) {
                enable_check = "true";
            }

            int duration = dvd_info->title_info[i].playback_time.hour * 3600 +
                    dvd_info->title_info[i].playback_time.minute * 60 +
                    dvd_info->title_info[i].playback_time.second;
            printf("\"fps\":%f,"
                   "\"main\":%s,"
                   "\"selectable\":%s,"
                   "\"angle\":%d,"
                   "\"duration\":%d,",
                   fps, main_title, enable_check, dvd_info->title_info[i].number_of_angles, duration);

            //视频
            int width = 0, height = 0, aspect_width = 0, aspect_height = 0;
            printf("\"video\":[");
            if (dvd_info->title_info[i].video_attributes) {
                char *standard = NULL;
                if (dvd_info->title_info[i].video_attributes->standard == vs_ntsc){
                    standard = "ntsc";
                    if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_720_480) {
                        width = 720;
                        height = 480;
                    }
                    else if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_704_480) {
                        width = 704;
                        height = 480;
                    }
                    else if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_352_480) {
                        width = 352;
                        height = 480;
                    }
                    else if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_352_240) {
                        width = 352;
                        height = 240;
                    }
                }
                else if (dvd_info->title_info[i].video_attributes->standard == vs_pal){
                    standard = "pal";
                    if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_720_576) {
                        width = 720;
                        height = 576;
                    }
                    else if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_704_576) {
                        width = 704;
                        height = 576;
                    }
                    else if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_352_576) {
                        width = 352;
                        height = 576;
                    }
                    else if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_352_288) {
                        width = 352;
                        height = 288;
                    }
                }

                if (dvd_info->title_info[i].video_attributes->aspect == va_4_3){
                    aspect_width = 4;
                    aspect_height = 3;
                }
                else if (dvd_info->title_info[i].video_attributes->aspect == va_16_9){
                    aspect_width = 16;
                    aspect_height = 9;
                }

                printf("{\"standard\":\"%s\","
                       "\"width\":%d,"
                       "\"height\":%d,"
                       "\"aspect_width\":%d,"
                       "\"aspect_height\":%d}",
                       standard, width, height, aspect_width, aspect_height);
            }
            printf("],");
            //音频
            printf("\"audio\":[");
            unsigned int audio_index;
            int gotAudio = 0;
            for (audio_index = 0; audio_index < dvd_info->title_info[i].number_of_audio; audio_index++) {
                if (dvd_info->title_info[i].audio_attributes) {
                    if(1 == gotAudio)
                    {
                        printf(",");
                    }
                    gotAudio = 1;
                    printf("{");
                    char* code = NULL;
                    printf("\"id\":%d,", dvd_info->title_info[i].audio_attributes[audio_index].id);
                    if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_ac3) {
                        code = "ac3";
                    }
                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_mpeg1) {
                        code = "mpeg1";
                    }
                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_mpeg2) {
                        code = "mpeg2";
                    }
                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_lpcm) {
                        code = "lpcm";
                    }
                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_dts) {
                        code = "dts";
                    }
                    else {
                        code = "unknow";
                    }
                    printf("\"standard\":\"%s\",", code);

                    printf("\"language\":\"%s\",", dvd_info->title_info[i].audio_attributes[audio_index].language);

                    if (dvd_info->title_info[i].audio_attributes[audio_index].sample_rate == asr_48kbps) {
                        printf("\"sample\":48000,");
                    }
                    printf("\"channels\":%d", dvd_info->title_info[i].audio_attributes[audio_index].number_of_channels);
                    printf("}");
                }
            }
            printf("],");
            //字幕
            printf("\"subtitle\":[");
            unsigned int subtitle_index;
            for (subtitle_index = 0; subtitle_index < dvd_info->title_info[i].number_of_subtitle; subtitle_index++) {
                printf("{\"id\":%d,", dvd_info->title_info[i].subtitle_attributes[subtitle_index].id);
                printf("\"language\":\"%s\"}", dvd_info->title_info[i].subtitle_attributes[subtitle_index].language);
            }
            printf("],");
            //chapter
            printf("\"chapter\":[");
            unsigned int chapter_index;
            for (chapter_index=0; chapter_index<dvd_info->title_info[i].number_of_chapter; chapter_index++) {
                int duration = dvd_info->title_info[i].chapter_attributes[chapter_index].playback_time.hour * 3600 +
                        dvd_info->title_info[i].chapter_attributes[chapter_index].playback_time.minute * 60 +
                        dvd_info->title_info[i].chapter_attributes[chapter_index].playback_time.second;
                if(0 == duration)
                {
                    continue;
                }
                if(chapter_index != 0)
                {
                    printf(",");
                }
                printf("%d", duration);
            }
            printf("]");

            printf("}");
        }
    }

    printf("]");
}

//void print_dvd_info(digiarty_dvd_info* dvd_info) {
//    if (dvd_info) {
//        uint32_t i;
//        for(i=0; i<dvd_info->number_of_title; i++) {

//            char *fps = "";
//            if (dvd_info->title_info[i].playback_time.fps == fps_25) {
//                fps = "25fps";
//            }
//            else if (dvd_info->title_info[i].playback_time.fps == fps_2997) {
//                fps = "29.97fps";
//            }

//            char *main_title = "";
//            if (dvd_info->title_info[i].is_main_title == 1) {
//                main_title = "MAIN TITLE";
//            }
//            char *enable_check = "Unenable check";
//            if (dvd_info->title_info[i].is_enable_title == 1) {
//                enable_check = "Enable check";
//            }

//            printf("title%02d  angle:%d  chapters:%2d  playback:%02d:%02d:%02d %s %s %s", i+1,
//                   dvd_info->title_info[i].number_of_angles,
//                   dvd_info->title_info[i].number_of_chapter,
//                   dvd_info->title_info[i].playback_time.hour,
//                   dvd_info->title_info[i].playback_time.minute,
//                   dvd_info->title_info[i].playback_time.second,
//                   fps, main_title, enable_check);

//            //视频
//            if (dvd_info->title_info[i].video_attributes) {
//                char *standard = NULL, *resolution = NULL, *aspect = NULL;
//                if (dvd_info->title_info[i].video_attributes->standard == vs_ntsc){
//                    standard = "ntsc";
//                    if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_720_480) {
//                        resolution = "720x480";
//                    }
//                    else if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_704_480) {
//                        resolution = "704x480";
//                    }
//                    else if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_352_480) {
//                        resolution = "352x480";
//                    }
//                    else if (dvd_info->title_info[i].video_attributes->resolution_ntsc == vrn_352_240) {
//                        resolution = "352x240";
//                    }
//                }
//                else if (dvd_info->title_info[i].video_attributes->standard == vs_pal){
//                    standard = "pal";
//                    if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_720_576) {
//                        resolution = "720x576";
//                    }
//                    else if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_704_576) {
//                        resolution = "704x576";
//                    }
//                    else if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_352_576) {
//                        resolution = "352x576";
//                    }
//                    else if (dvd_info->title_info[i].video_attributes->resolution_pal == vrp_352_288) {
//                        resolution = "352x288";
//                    }
//                }

//                if (dvd_info->title_info[i].video_attributes->aspect == va_4_3){
//                    aspect = "4:3";
//                }
//                else if (dvd_info->title_info[i].video_attributes->aspect == va_16_9){
//                    aspect = "16:9";
//                }

//                printf(" standard[%s] resolution[%s] aspect[%s]\n", standard, resolution, aspect);
//            }
//            //音频
//            unsigned int audio_index;
//            for (audio_index = 0; audio_index < dvd_info->title_info[i].number_of_audio; audio_index++) {
//                if (dvd_info->title_info[i].audio_attributes) {

//                    printf("id:%X\t", dvd_info->title_info[i].audio_attributes[audio_index].id);
//                    if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_ac3) {
//                        printf("code:ac3\t");
//                    }
//                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_mpeg1) {
//                        printf("code:mpeg1\t");
//                    }
//                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_mpeg2) {
//                        printf("code:mpeg2\t");
//                    }
//                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_lpcm) {
//                        printf("code:lpcm\t");
//                    }
//                    else if (dvd_info->title_info[i].audio_attributes[audio_index].coding_mode == acm_dts) {
//                        printf("code:dts\t");
//                    }
//                    else {
//                        printf("code:unknow\t");
//                    }

//                    printf("language:%s\t", dvd_info->title_info[i].audio_attributes[audio_index].language);

//                    if (dvd_info->title_info[i].audio_attributes[audio_index].sample_rate == asr_48kbps) {
//                        printf("48kbps\t");
//                    }
//                    printf("%dCH\t", dvd_info->title_info[i].audio_attributes[audio_index].number_of_channels);
//                    printf("\n");
//                }
//            }
//            //字幕
//            unsigned int subtitle_index;
//            for (subtitle_index = 0; subtitle_index < dvd_info->title_info[i].number_of_subtitle; subtitle_index++) {
//                printf("\tsubtitle->%02d\t", dvd_info->title_info[i].subtitle_attributes[subtitle_index].id);
//                printf("language:%s\t", dvd_info->title_info[i].subtitle_attributes[subtitle_index].language);
//                printf("\n");
//            }
//            //chapter
//            unsigned int chapter_index;
//            for (chapter_index=0; chapter_index<dvd_info->title_info[i].number_of_chapter; chapter_index++) {
//                printf("\tChapter%02d %02d:%02d:%02d\n", chapter_index + 1,
//                       dvd_info->title_info[i].chapter_attributes[chapter_index].playback_time.hour,
//                       dvd_info->title_info[i].chapter_attributes[chapter_index].playback_time.minute,
//                       dvd_info->title_info[i].chapter_attributes[chapter_index].playback_time.second );
//            }
//        }
//    }
//}

int main(int argc, char *argv[])
{
//    if (argc != 3) {
//        return 1;
//    }

    digiarty_dvd_info *dvd_info = get_dvd_info("F:/My DVD"/*argv[1]*/, 0/*atoi(argv[2])*/);
    if (dvd_info) {
        print_dvd_info(dvd_info);
        free_dvd_info(dvd_info);
    }
}
