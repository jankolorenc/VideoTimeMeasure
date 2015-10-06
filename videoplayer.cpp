#include "videoplayer.h"
#include "intervaltimestamp.h"

#ifdef __cplusplus
extern "C" {
    #endif
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #ifdef __cplusplus
}
#endif

uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

int our_get_buffer(struct AVCodecContext *c, AVFrame *pic) {
    int ret = avcodec_default_get_buffer(c, pic);
    uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    return ret;
}

void our_release_buffer(struct AVCodecContext *c, AVFrame *pic) {
    if(pic) av_freep(&pic->opaque);
    avcodec_default_release_buffer(c, pic);
}

VideoPlayer::VideoPlayer(QObject *parent) :
    QObject(parent)
{
    options = NULL;
    pFormatCtx = NULL;
    pCodecCtx = NULL;
    pCodec = NULL;
    videoStream = -1;
    pFrame = NULL;
    pFrameRGB = NULL;
    buffer = NULL;
    sws_ctx = NULL;

    imagesBufferOldest = -1;
    imagesBufferNewest = -1;
    imagesBufferCurrent = -1;
    backSeekFactor = 1;
    sliderFactor = 1;
    stopPlayerDts = 0xffffffffffffffff;

    selectCellRow = -1;
    selectCellColumn = -1;

    for(int i = 0; i < IMAGES_BUFFER_SIZE; i++) imagesBuffer[i].image = NULL;

    connect(&playTimer, SIGNAL(timeout()), this, SLOT(on_playTimerTimeout()));
}

VideoPlayer::~VideoPlayer(){
    freeDecodingFrameBuffers();
    closeVideoFile();
}

bool VideoPlayer::loadFile(QString fileName){
    // Open video file
    QByteArray fileNameByteArray = fileName.toLocal8Bit();
    int result = avformat_open_input(&pFormatCtx,fileNameByteArray.data(), NULL, options);
    if(result < 0){
        char error_string[200];
        av_strerror(result, error_string, 200);
        //showError(tr("Couldn't open file %1: %2").arg(fileName).arg(QString(error_string)));
        return FALSE;
    }

    if(avformat_find_stream_info(pFormatCtx, options)<0){
        //showError(tr("Couldn't find stream information in video"));
        return FALSE;
    }

    av_dump_format(pFormatCtx, 0, fileNameByteArray.data(), 0);

    // Find the first video stream
    videoStream=-1;
    for(uint i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1){
        //showError(tr("Didn't find a video stream"));
        return FALSE;
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        //showError(tr("Unsupported codec"));
        return FALSE;
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, options)<0){
        //showError(tr("Could not open codec"));
        return FALSE;
    }

    pCodecCtx->get_buffer = our_get_buffer;
    pCodecCtx->release_buffer = our_release_buffer;

    allocateDecodingFrameBuffers();

    return TRUE;
}

void VideoPlayer::closeVideoFile(){
    // Close the codec
    if (pCodecCtx != NULL){
        avcodec_close(pCodecCtx);
        pCodecCtx = NULL;
    }
    // Close the video file
    if (pFormatCtx != NULL){
        avformat_close_input(&pFormatCtx);
        pFormatCtx = NULL;
    }
}

void VideoPlayer::allocateDecodingFrameBuffers(){

    for(int i = 0; i < IMAGES_BUFFER_SIZE; i++){
        if (imagesBuffer[i].image != NULL){
            delete imagesBuffer[i].image;
            imagesBuffer[i].image = NULL;
        }
    }

    imagesBufferOldest = -1;
    imagesBufferNewest = -1;
    imagesBufferCurrent = -1;

    // Allocate video frame
    pFrame=avcodec_alloc_frame();
    if(pFrame==NULL){
        // showError(tr("Couldn't not allocate frame"));
        return;
    }

    // Allocate an AVFrame structure
    pFrameRGB=avcodec_alloc_frame();
    if(pFrameRGB==NULL){
        // showError(tr("Couldn't not allocate RGB frame"));
        return;
    }

    int numBytes;
    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    sws_ctx = sws_getContext (pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
                              pCodecCtx->height, PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
}

void VideoPlayer::freeDecodingFrameBuffers(){
    if (sws_ctx != NULL){
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }
    if (buffer != NULL){
        av_free(buffer);
        buffer = NULL;
    }
    // Free the RGB image
    if (pFrameRGB != NULL){
        av_free(pFrameRGB);
        pFrameRGB = NULL;
    }
    // Free the YUV frame
    if (pFrame != NULL){
        av_free(pFrame);
        pFrame = NULL;
    }
}

AVRational VideoPlayer::synchronizeVideo(AVFrame *src_frame, AVRational pts) {
    AVRational frame_delay;
    if(pts.num != 0) {
        /* if we have pts, set video clock to it */
        video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = video_clock;
    }
    /* update the video clock */
    frame_delay = pFormatCtx->streams[videoStream]->codec->time_base;
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay = av_add_q(frame_delay, av_mul_q(av_make_q(src_frame->repeat_pict, 1), av_mul_q(frame_delay, av_make_q(1, 2))));
    video_clock = av_add_q(video_clock, frame_delay);
    return pts;
}

bool VideoPlayer::readNextFrame(){
    if (pFormatCtx == NULL) return FALSE;

    AVPacket packet;
    int frameFinished = 0;

    while(av_read_frame(pFormatCtx, &packet)>=0) {
        if(packet.stream_index==videoStream) {
            pts = av_make_q(0, 1);
            // Save global pts to be stored in pFrame in first call
            global_video_pkt_pts = packet.pts;
            // Is this a packet from the video stream?

            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if(packet.dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
                pts = av_make_q(*(uint64_t *)pFrame->opaque, 1);
            } else if(packet.dts != AV_NOPTS_VALUE) {
                pts = av_make_q(packet.dts, 1);
            } else {
                pts = av_make_q(0, 1);
            }
            pts = av_mul_q(pts, pFormatCtx->streams[videoStream]->time_base);
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        if (frameFinished){
            pts = synchronizeVideo(pFrame, pts);
            bufferCurrentFrame();
            break;
        }
    }

    return frameFinished;
}

void VideoPlayer::bufferCurrentFrame(){
    // Convert the image from its native format to RGB
    sws_scale (sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0,
               pCodecCtx->height, pFrameRGB->data,pFrameRGB->linesize);

    imagesBufferNewest = (imagesBufferNewest + 1) % IMAGES_BUFFER_SIZE;

    // imagesBufferNewest reached oldest indices in circular buffer
    if (imagesBufferNewest == imagesBufferOldest)
        imagesBufferOldest = (imagesBufferOldest + 1) % IMAGES_BUFFER_SIZE;
    if (imagesBufferNewest == imagesBufferCurrent)
        imagesBufferCurrent = (imagesBufferCurrent + 1) % IMAGES_BUFFER_SIZE;

    if (imagesBuffer[imagesBufferNewest].image == NULL)
        imagesBuffer[imagesBufferNewest].image = new QImage(pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB888);
    //fill QImage
    for(int y=0 ; y<pCodecCtx->height; y++){
        memcpy(imagesBuffer[imagesBufferNewest].image->scanLine(y),
               (void *)((pFrameRGB->data[0])+(y*pFrameRGB->linesize[0])),
               pCodecCtx->width*3);
    }

    imagesBuffer[imagesBufferNewest].pts = pts;
    imagesBuffer[imagesBufferNewest].dts = pFrame->pkt_dts;

    if (imagesBufferCurrent == -1) imagesBufferCurrent = imagesBufferNewest;
    if (imagesBufferOldest == -1) imagesBufferOldest = imagesBufferNewest;
}

void VideoPlayer::seek(AVRational targetPts, bool exactSeek, bool stayOneImageBack){
    //limit backseek factor for case when ffmpeg cannot seek
    while(backSeekFactor < MAX_BACK_SEEK_FACTOR){
        AVRational backSeekDuration;
        if (exactSeek) backSeekDuration = av_div_q(av_make_q(BACK_SEEK_FRAMES * backSeekFactor, 1), pFormatCtx->streams[videoStream]->r_frame_rate);
        else backSeekDuration = av_make_q(0, 1);
        int64_t seekTarget = av_q2d(av_mul_q(av_sub_q(targetPts, backSeekDuration), av_make_q(AV_TIME_BASE, 1)));

        // hope this is fix for mjpeg back seek causing crash
        if (pFormatCtx->start_time >=0 && seekTarget < 0) seekTarget = 0;

        seekTarget = av_rescale_q(seekTarget, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);

        int result = av_seek_frame(pFormatCtx, videoStream, seekTarget, AVSEEK_FLAG_BACKWARD);
        if (result >= 0){
            avcodec_flush_buffers(pFormatCtx->streams[videoStream]->codec);
            // flush imagesBuffer
            imagesBufferCurrent = imagesBufferNewest = imagesBufferOldest = -1;
            video_clock = av_make_q(0, 1);

            // read and buffer previous images
            if (exactSeek){
                if (stayOneImageBack) while (readNextFrame() && av_cmp_q(imagesBuffer[imagesBufferNewest].pts, targetPts) == -1);
                else while (readNextFrame() && av_cmp_q(imagesBuffer[imagesBufferNewest].pts, targetPts) != 1);
                if (imagesBufferNewest != imagesBufferOldest){
                    imagesBufferCurrent = (imagesBufferNewest -1 + IMAGES_BUFFER_SIZE) % IMAGES_BUFFER_SIZE;
                    break;
                }
                else{
                    backSeekFactor++;
                }
            }
            else{
                readNextFrame();
                readNextFrame();
                break;
            }
        }
    }

    // restart backseek factor to allow seek to another target
    if (backSeekFactor >= MAX_BACK_SEEK_FACTOR) backSeekFactor = 1;
}

bool VideoPlayer::stepForward(int jumpImages)
{
    for(int i = 0; i < jumpImages; i++)
        if (imagesBufferCurrent != -1 && imagesBufferCurrent != imagesBufferNewest){
            imagesBufferCurrent = (imagesBufferCurrent + 1) % IMAGES_BUFFER_SIZE;
            if (i == jumpImages - 1) showCurrentFrame();
            if (imagesBufferCurrent == imagesBufferNewest) readNextFrame();
        }
        else return false;

    return true;
}

bool VideoPlayer::isStopReached()
{
    return imagesBufferCurrent > -1 && imagesBuffer[imagesBufferCurrent].dts >= stopPlayerDts;
}

void VideoPlayer::clearState(){
    closeVideoFile();
    freeDecodingFrameBuffers();
    imagesBufferCurrent = imagesBufferNewest = imagesBufferOldest = -1;
    backSeekFactor = 1;
    sliderFactor = 1;
    stopPlayerDts = 0xffffffffffffffff;
}

int64_t VideoPlayer::streamDuration()
{
    if (isEmpty()) return 0;
    return av_rescale_q(pFormatCtx->duration, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);
}

double VideoPlayer::framerate()
{
    if (isEmpty()) return 0;
    return av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate);
}

double VideoPlayer::durationSeconds()
{
    if (isEmpty()) return 0;
    return pFormatCtx->duration * av_q2d(AV_TIME_BASE_Q);
}

VideoImage *VideoPlayer::currentImage(){
    if (imagesBufferCurrent == -1 || imagesBuffer[imagesBufferCurrent].image == NULL) return NULL;
    return imagesBuffer + imagesBufferCurrent;
}

int64_t VideoPlayer::startTime(){
    if (isEmpty()) return 0;
    return pFormatCtx->streams[videoStream]->start_time;
}

AVRational VideoPlayer::timebase(){
    if (isEmpty()) return av_make_q(0, 1);
    return pFormatCtx->streams[videoStream]->time_base;
}

bool VideoPlayer::stepReverse(int jumpImages)
{
    if (isEmpty()) return false;

    for(int i = 0; i < jumpImages; i++){
        if (imagesBufferCurrent == -1) return false;
        if (imagesBufferCurrent != imagesBufferOldest){
            imagesBufferCurrent = (imagesBufferCurrent - 1 + IMAGES_BUFFER_SIZE) % IMAGES_BUFFER_SIZE;
        }
        else{
            int64_t start_time = av_rescale_q(pFormatCtx->start_time, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);
            if (imagesBuffer[imagesBufferCurrent].dts > start_time){
                seek(imagesBuffer[imagesBufferCurrent].pts, true, true);
            }
        }
    }
    showCurrentFrame();

    return true;
}

bool VideoPlayer::isEmpty(){
    return pFormatCtx == NULL || videoStream == -1;
}

bool VideoPlayer::isPlaying(){
    return playTimer.isActive();
}

void VideoPlayer::play(IntervalTimestamp *stop, int selectCellRow, int selectCellColumn){
    if (stop != NULL && stop->isValid) stopPlayerDts = stop->dts;
    else stopPlayerDts = 0xffffffffffffffff;
    this->selectCellRow = selectCellRow;
    this->selectCellColumn = selectCellColumn;
    double timeout = 1 / framerate() * 1000;
    playTimer.start(timeout);
}

void VideoPlayer::stop(){
    if (isPlaying()){
        playTimer.stop();
        if (isStopReached() && selectCellRow > -1 && selectCellColumn > -1) stopped(selectCellRow, selectCellColumn);
        else stopped();
        selectCellRow = selectCellColumn = -1;
    }
}

void VideoPlayer::on_playTimerTimeout(){
    if (!stepForward() || isStopReached()) stop();
}
