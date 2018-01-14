//
// Created by wanbin on 2017/10/15.
//



#ifndef FFMPEGMUSIC_FFMPEGAUDIO_H
#define FFMPEGMUSIC_FFMPEGAUDIO_H


#include <queue>
#include <SLES/OpenSLES_Android.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include "log.h"
#include <android/log.h>
#include <pthread.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
class FFmpegAudio{

public:
    FFmpegAudio();


    ~FFmpegAudio();

    int get(AVPacket *packet);

    int put(AVPacket*packet);

    void play();

    void stop();
    int createPlayer();
    void setAvCodecContext(AVCodecContext * codecContext);
//成员变量
public:
    //是否正在播放
    int isPlay;
    //流索引
    int index;
    //音频队列
    std::queue<AVPacket *> queue;

    //处理线程
    pthread_t p_playid;

    //解码器上下文
    AVCodecContext * codec;
    //同步锁
    pthread_mutex_t mutex;

    //条件变量
    pthread_cond_t cond;


    /**
   * 新增
   */
    SwrContext *swrContext;
    uint8_t *out_buffer;
    int out_channer_nb;
//    相对于第一帧时间
    double clock;

    AVRational time_base;

    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
};
};

#endif //FFMPEGMUSIC_FFMPEGAUDIO_H