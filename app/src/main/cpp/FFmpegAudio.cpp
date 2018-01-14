//
// Created by wanbin on 2017/10/15.
//



#include "FFmpegAudio.h"

FFmpegAudio::FFmpegAudio() {
    clock =0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}
/**
 * 解码后的数据
 * @param audio
 */
int getPCM(FFmpegAudio *audio){
    int frameCount=0;
    int got_frame;
    int size;
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    while (audio->isPlay) {


        size=0;
        audio->get(packet);
        if (packet->pts != AV_NOPTS_VALUE) {
            audio->clock = av_q2d(audio->time_base) * packet->pts;
        }
//            解码  mp3   编码格式frame----pcm   frame
        avcodec_decode_audio4(audio->codec, frame, &got_frame, packet);
        if (got_frame) {

            swr_convert(audio->swrContext, &audio->out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
//                缓冲区的大小
            size = av_samples_get_buffer_size(NULL, audio->out_channer_nb, frame->nb_samples,
                                              AV_SAMPLE_FMT_S16, 1);
            break;
        }
    }
    av_free(packet);
    av_frame_free(&frame);
    return size;
}

//第一次主动调用在调用线程
//之后在新线程中回调
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    FFmpegAudio  *audio = (FFmpegAudio *) context;
    int datalen = getPCM(audio);
    if (datalen > 0) {
        double time = datalen/((double) 44100 *2 * 2);
        LOGE("数据长度%d  分母%d  值%f 通道数%d",datalen,44100 *2 * 2,time,audio->out_channer_nb);
        audio->clock = audio->clock +time;
        LOGE("当前一帧声音时间%f   播放时间%f",time,audio->clock);
        (*bq)->Enqueue(bq, audio->out_buffer, datalen);
        LOGE("播放 %d ",audio->queue.size());
    } else
        LOGE("解码错误");
}

int createFFmpeg(FFmpegAudio *audio){
//    mp3  里面所包含的编码格式   转换成  pcm   SwcContext
    audio->swrContext = swr_alloc();

    int length=0;
    int got_frame;
//    44100*2
    audio->out_buffer = (uint8_t *) av_malloc(44100 * 2);
    uint64_t  out_ch_layout=AV_CH_LAYOUT_STEREO;
//    输出采样位数  16位
    enum AVSampleFormat out_formart=AV_SAMPLE_FMT_S16;
//输出的采样率必须与输入相同
    int out_sample_rate = audio->codec->sample_rate;


    swr_alloc_set_opts( audio->swrContext, out_ch_layout, out_formart, out_sample_rate,
                        audio->codec->channel_layout, audio->codec->sample_fmt,  audio->codec->sample_rate, 0,
                        NULL);

    swr_init( audio->swrContext);
//    获取通道数  2
    audio->out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    LOGE("------>通道数%d  ", audio->out_channer_nb);
    return 0;
}
int FFmpegAudio::createPlayer() {
    SLresult result;
    // 创建引擎engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现引擎engineObject
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 获取引擎接口engineEngine
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineEngine);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 创建混音器outputMixObject
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0,
                                              0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    // 实现混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return 0;
    }
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == result) {
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &settings);
    }


    //======================
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
//   新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};
//    设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    //先讲这个
    (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &slDataSource,
                                       &audioSnk, 2,
                                       ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);

//    注册回调缓冲区 //获取缓冲队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    //缓冲接口回调
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
//    获取音量接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);

//    获取播放状态接口
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    bqPlayerCallback(bqPlayerBufferQueue, this);
    return 1;
}


void FFmpegAudio::setAvCodecContext(AVCodecContext *codecContext) {
    codec = codecContext;
    createFFmpeg(this);
}

//消费者
int FFmpegAudio::get(AVPacket *packet) {
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (!queue.empty()) {
//            从队列取出一个packet   clone一个 给入参对象
            if (av_packet_ref(packet, queue.front())) {
                break;
            }
//            取成功了  弹出队列  销毁packet
            AVPacket *pkt = queue.front();
            queue.pop();
            LOGE("取出一 个音频帧%d",queue.size());
            av_free(pkt);
            break;
        } else {
//            如果队列里面没有数据的话  一直等待阻塞
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}
//生产者
int FFmpegAudio::put(AVPacket *packet) {
    AVPacket *packet1 = (AVPacket *) av_mallocz(sizeof(AVPacket));
    if (av_packet_ref(packet1, packet)) {
//        克隆失败
        return 0;
    }

    pthread_mutex_lock(&mutex);
    queue.push(packet1);
    LOGE("压入一帧音频数据  队列%d ",queue.size());
//    给消费者解锁
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

void *play_audio(void *arg){
    FFmpegAudio *audio = (FFmpegAudio  *) arg;
    audio->createPlayer();
    pthread_exit(0);
}
void FFmpegAudio::play() {
    isPlay = 1;
    pthread_create(&p_playid,NULL, play_audio, this);

}

void FFmpegAudio::stop() {
    LOGE("声音暂停");
    //因为可能卡在 deQueue
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(p_playid, 0);
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;

        bqPlayerBufferQueue = 0;
        bqPlayerVolume = 0;
    }

    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }

    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineEngine = 0;
    }
    if (swrContext)
        swr_free(&swrContext);
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGE("AUDIO clear");
}

FFmpegAudio::~FFmpegAudio() {
    if (out_buffer) {
        free(out_buffer);
    }
    for (int i = 0; i < queue.size(); ++i) {
        AVPacket *pkt = queue.front();
        queue.pop();
        LOGE("销毁音频帧%d",queue.size());
        av_free(pkt);
    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}



