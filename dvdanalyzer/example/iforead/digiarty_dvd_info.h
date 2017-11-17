#ifndef DIGIARTY_DVD_INFO_H
#define DIGIARTY_DVD_INFO_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * 视频制式
 */
typedef enum {
	vs_ntsc,
	vs_pal,
}vidoe_standard;

/*
 * 画面比例
 */
typedef enum {
	va_4_3,
	va_reserved_1,
	va_reserved_2,
	va_16_9,
}video_aspect;

/*
 * NTSC画面大小
 */
typedef enum {
	vrn_720_480,
	vrn_704_480,
	vrn_352_480,
	vrn_352_240,
}video_resolution_ntsc;

/*
 * PAL画面大小
 */
typedef enum {
	vrp_720_576,
	vrp_704_576,
	vrp_352_576,
	vrp_352_288,
}video_resolution_pal;

/*
 * 视频属性
 */
typedef struct {
	vidoe_standard standard;
	video_aspect aspect;
	union {
		video_resolution_ntsc resolution_ntsc;
		video_resolution_pal resolution_pal;
	};
}digiarty_video_attributes;

/*
 * 音频编码
 */
typedef enum {
	acm_ac3,
	acm_reserved_1,
	acm_mpeg1,
	acm_mpeg2,
	acm_lpcm,
	acm_reserved_2,
	acm_dts,
	acm_reserved_3,
}audio_coding_mode;

/*
 * 采样频率
 */
typedef enum {
	asr_48kbps,
}audio_sample_rate;

/*
 * 音频属性
 */
typedef struct {
	unsigned int id;
	audio_coding_mode coding_mode;
	char language[4];
	audio_sample_rate sample_rate;
	unsigned int number_of_channels;
}digiarty_audio_attributes;

/*
 * 字幕属性
 */
typedef struct {
	unsigned int id;
	char language[4];
}digiarty_subtitle_attributes;

/*
 * fps
 */
typedef enum {
	fps_25 = 1,
	fps_2997 = 3,
}digiarty_fps;

/*
 * time
 */
typedef struct {
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
	digiarty_fps fps;
}dvd_time;

/*
 * chapter属性
 */
typedef struct {
	dvd_time playback_time;
}digiarty_chapter_attributes;

/*
 * title属性
 */
typedef struct {
	dvd_time playback_time;

	unsigned int number_of_angles;
	digiarty_video_attributes *video_attributes;
	unsigned int number_of_audio;
	digiarty_audio_attributes *audio_attributes;
	unsigned int number_of_subtitle;
	digiarty_subtitle_attributes *subtitle_attributes;

	unsigned int number_of_chapter;
	digiarty_chapter_attributes *chapter_attributes;

	int is_main_title;
	int is_enable_title;

}digiarty_title_info;

typedef struct {
	int is_disney_fake;
	unsigned int number_of_title;
	digiarty_title_info* title_info;
    int is_css;
    int track_model;    // 1:UDF/2:iso/-1:未知的
}digiarty_dvd_info;

digiarty_dvd_info* get_dvd_info(const char* dvd_path, int model);
void free_dvd_info(digiarty_dvd_info* dvd_info);

typedef void (*FDebugLog)(const char* log);
void SetDebugLogFunction(FDebugLog DebugLog);

#ifdef	__cplusplus
}
#endif
#endif //#define DIGIARTY_DVD_INFO_H
