//
// Created by wanbin on 2017/10/15.
//



#ifndef FFMPEGMUSIC_FFMPEGVEDIO_H
#define FFMPEGMUSIC_FFMPEGVEDIO_H


#include <queue>
#include "FFmpegAudio.h"

extern "C"{


#include <libavcodec/avcodec.h>
#include <pthread.h>
#include <android/log.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include "FFmpegVideo.h"
#include "log.h"
#include <libavutil/time.h>

class FFmpegVideo{

public:
 FFmpegVideo();
    ~FFmpegVideo();

    int get(AVPacket *packet);

    int put(AVPacket*packet);

    void play();

    void stop();

    void setAvCodecContext(AVCodecContext * codecContext);
    /**
     * 设置回调接口
     * @param call
     */
    void setPlayCall(void (*call)(AVFrame* frame));

    double synchronize(AVFrame *frame, double play);

    void setAudio(FFmpegAudio *audio);
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
 * ------新增--------
 */
//头文件写在"extend"C外
    FFmpegAudio* audio;
    AVRational time_base;
    double  clock;
};
};

#endif //FFMPEGMUSIC_FFMPEGVEDIO_H