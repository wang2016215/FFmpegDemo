//
// Created by wanbin on 2017/10/15.
//

#ifndef FFMPEGMUSIC_LOG_H
#define FFMPEGMUSIC_LOG_H

#endif //FFMPEGMUSIC_LOG_H
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);