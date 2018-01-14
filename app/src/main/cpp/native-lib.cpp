#include <jni.h>
#include <string>
#include <android/log.h>

#include "FFmpegAudio.h"
#include "FFmpegVideo.h"
#include <android/native_window_jni.h>
extern "C" {

ANativeWindow *window=0;
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"

#include <unistd.h>
#include "FFmpegVideo.h"
#include "FFmpegAudio.h"
#include "log.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <assert.h>



const char *path;

FFmpegVideo *video;
FFmpegAudio * audio;

pthread_t  p_tid;
int isPlay = 0;


void call_video_play(AVFrame *frame){
    if (!window) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }

    LOGE("绘制 宽%d,高%d",frame->width,frame->height);
    LOGE("绘制 宽%d,高%d  行字节 %d ",window_buffer.width,window_buffer.height, frame->linesize[0]);
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}

//解码函数
void *process(void *args){
    LOGE("开启解码线程");
    //1.注册组件
    av_register_all();
    avformat_network_init();
    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if(avformat_open_input(&pFormatCtx,path,NULL,NULL) != 0){
        LOGE("%s","打开输入视频文件失败");
    }
    //3.获取视频信息
    if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
        LOGE("%s","获取视频信息失败");
    }

    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int i = 0;
    for(; i < pFormatCtx->nb_streams;i++){
        //4.获取视频解码器
        AVCodecContext *pCodeCtx = pFormatCtx->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);

        AVCodecContext *codec = avcodec_alloc_context3(pCodec);
        avcodec_copy_context(codec, pCodeCtx);
        if(avcodec_open2(codec,pCodec,NULL) < 0){
            LOGE("%s","解码器无法打开");
            continue;
        }
        //根据类型判断，是否是视频流
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            /*找到视频流*/
            video->setAvCodecContext(codec);
            video->index = i;
            video->time_base = pFormatCtx->streams[i]->time_base;
            if (window)
                ANativeWindow_setBuffersGeometry(window, video->codec->width,
                                                 video->codec->height,
                                                 WINDOW_FORMAT_RGBA_8888);

        } else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio->setAvCodecContext(codec);
            audio->index = i;
            audio->time_base=pFormatCtx->streams[i]->time_base;
        }

    }

//    开启 音频 视频  播放的死循环
    video->setAudio(audio);
    video->play();
    audio->play();
    isPlay = 1;
//    解码packet

    //编码数据
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
//    解码完整个视频 子线程
    int ret;
    while (isPlay ) {
//        如果这个packet  流索引 等于 视频流索引 添加到视频队列
        ret = av_read_frame(pFormatCtx, packet);
        if (ret == 0) {
            if (video && video->isPlay && packet->stream_index == video->index) {
                video->put(packet);
            } else  if (audio&& audio->isPlay && packet->stream_index == audio->index) {
                audio->put(packet);
            }
            av_packet_unref(packet);
        } else if(ret == AVERROR_EOF) {
//            读完了
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
                if (video->queue.empty() && audio->queue.empty()) {
                    break;
                }
//                LOGI("等待播放完成");
                av_usleep(10000);
            }
        }

    }
//    视频解码完     可能视频播放完了   也可能视频没播放完成







    isPlay = 0;
    if (video && video->isPlay) {
        video->stop();
    }
    if (audio && audio->isPlay) {
        audio->stop();
    }
    av_free_packet(packet);
    avformat_free_context(pFormatCtx);
    pthread_exit(0);

}



extern "C"
JNIEXPORT void JNICALL
Java_com_example_wanbin_ffmpegmusic_DavidPlayer_play(JNIEnv *env, jobject instance, jstring path_) {
    path = env->GetStringUTFChars(path_, 0);

    // TODO

    //实例化对象

    video = new FFmpegVideo;
    audio = new FFmpegAudio;
    video->setPlayCall(call_video_play);
    pthread_create(&p_tid,NULL,process,NULL);

}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wanbin_ffmpegmusic_DavidPlayer_display(JNIEnv *env, jobject instance,
                                                        jobject surface) {

    // TODO
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    if (video && video->codec) {
        ANativeWindow_setBuffersGeometry(window, video->codec->width, video->codec->height,
                                         WINDOW_FORMAT_RGBA_8888);
    }

}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wanbin_ffmpegmusic_DavidPlayer_release(JNIEnv *env, jobject instance) {

    // TODO
    if (isPlay) {
        isPlay = 0;
        pthread_join(p_tid, 0);
    }
    if (video) {
        if (video->isPlay) {
            video->stop();
        }
        delete (video);
        video = 0;
    }
    if (audio) {
        if (audio->isPlay) {
            audio->stop();
        }
        delete (audio);
        audio = 0;

    }
}

}