#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
#include <QTimer>
#include "videoimage.h"
#include "intervaltimestamp.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif

//#define IMAGES_BUFFER_SIZE 10
#define IMAGES_BUFFER_SIZE 20
#define BACK_SEEK_FRAMES 12
#define MAX_BACK_SEEK_FACTOR 20

class VideoPlayer : public QObject
{
    Q_OBJECT
private:
    AVDictionary **options;
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    int videoStream;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    uint8_t *buffer;
    struct SwsContext *sws_ctx;
    AVRational video_clock;
    int backSeekFactor;
    AVRational pts;

    VideoImage imagesBuffer[IMAGES_BUFFER_SIZE];
    int imagesBufferOldest;
    int imagesBufferNewest;
    int imagesBufferCurrent;

    AVRational stopPlayerPts;

    QTimer playTimer;
    int selectCellRow;
    int selectCellColumn;

public:
    explicit VideoPlayer(QObject *parent = 0);
    ~VideoPlayer();

    bool loadFile(QString fileName);
    void closeVideoFile();
    void allocateDecodingFrameBuffers();
    void freeDecodingFrameBuffers();
    AVRational synchronizeVideo(AVFrame *src_frame, AVRational pts);
    bool readNextFrame();
    void bufferCurrentFrame();
    void seek(AVRational targetPts, bool exactSeek);
    bool stepForward(int jumpImages = 1);
    bool isStopReached();
    void clearState();
    int64_t streamDuration();
    double framerate();
    double durationSeconds();
    VideoImage *currentImage();
    int64_t startTime();
    bool stepReverse(int jumpImages = 1);
    bool isEmpty();
    AVRational timebase();
    void play(IntervalTimestamp *stop = NULL, int selectCellRow = -1, int selectCellColumn = -1);
    void stop();
    bool isPlaying();

signals:
    void showCurrentFrame();
    void stopped(int selectCellRow = -1, int selectCellColumn = -1);

private slots:
    void on_playTimerTimeout();

};

#endif // VIDEOPLAYER_H
