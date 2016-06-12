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

/**
 * @brief The VideoPlayer class
 * FFMpeg wrapper and video player state holding class
 */
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

    /**
     * @brief multiplier of BACK_SEEK_FRAMES to jump before seeked time.
     * It is estimation of GOP so seek jumps to iframe before seeked time
     */
    int backSeekFactor;

    /**
     * @brief buffer to store decoded images for short and fast back jumps
     */
    VideoImage imagesBuffer[IMAGES_BUFFER_SIZE];

    /**
     * @brief imagesBufferOldest
     * Index of oldest image in imagesBuffer
     */
    int imagesBufferOldest;

    /**
     * @brief imagesBufferNewest
     * Index of newest image in imagesBuffer
     */
    int imagesBufferNewest;

    /**
     * @brief imagesBufferCurrent
     * Index of current image in imagesBuffer
     */
    int imagesBufferCurrent;

    /**
     * @brief timestamp where player will stop playing
     */
    AVRational stopPlayerPts;

    /**
     * @brief timer to display nex image when plaiyng video
     */
    QTimer playTimer;

    /**
     * @brief row to select in table when player stops
     */
    int selectCellRow;

    /**
     * @brief column to select in table when player stops
     */
    int selectCellColumn;

    /**
     * @brief allocate decoding buffers
     */
    void allocateDecodingBuffers();

    /**
     * @brief deallocate decoding buffers
     */
    void freeDecodingBuffers();

    /**
     * @brief save current frame to images buffer
     */
    void bufferCurrentFrame();

    /**
     * @brief stop timestamp (stopPlayerPts) reached test
     * @return true if current image is at stop timestamp
     */
    bool isStopReached();

public:
    explicit VideoPlayer(QObject *parent = 0);
    ~VideoPlayer();

    /**
     * @brief load video file
     * @param fileName
     * @return true when loaded
     */
    bool loadFile(QString fileName);

    /**
     * @brief close video file and deallocate file related data structures
     */
    void closeVideoFile();

    /**
     * @brief read next frame from video file and store it to images buffer
     * @return true if reading finished, false if need to be called again
     */
    bool readNextFrame();

    /**
     * @brief seek video file
     * @param targetPts target timestamp
     * @param exactSeek is exact timestamp seek is required.
     * Seek will jump to nearest iframe in exact seeking is not required.
     */
    void seek(AVRational targetPts, bool exactSeek);

    /**
     * @brief step forward
     * @param jumpImages number of images to jump
     * @return false if it is impossible to jump
     */
    bool stepForward(int jumpImages = 1);

    /**
     * @brief close video file, free buffers and reset current position
     */
    void clearState();

    /**
     * @brief stream duration in FFMpeg units
     * @return stream duration
     */
    int64_t getStreamDuration();

    /**
     * @brief stream framerate
     * @return framerate
     */
    double getFramerate();

    /**
     * @brief video duration in seconds
     * @return video duration
     */
    double getDurationSeconds();

    /**
     * @brief get current image
     * @return current image
     */
    VideoImage *getCurrentImage();


    /**
     * @brief get video start time in FFMpeg units
     * @return start time
     */
    int64_t getStartTime();

    /**
     * @brief step backward
     * @param jumpImages number of images to jump
     * @return false if it is impossible to jump
     */
    bool stepReverse(int jumpImages = 1);

    /**
     * @brief test whether player contains video
     * @return true if video is loaded in the player
     */
    bool isEmpty();

    /**
     * @brief get video timebase in FFMpeg units
     * @return timebase
     */
    AVRational getTimebase();

    /**
     * @brief play video till stop timestamp is reached and restore selection of given cell
     * @param stop timestamp
     * @param selectCellRow row of selected cell when playing stops
     * @param selectCellColumn column of selected cell when playing stops
     */
    void play(IntervalTimestamp *stop = NULL, int selectCellRow = -1, int selectCellColumn = -1);

    /**
     * @brief stop playing video
     */
    void stop();

    /**
     * @brief test whether player is playing
     * @return true vhen playing video
     */
    bool isPlaying();

signals:
    /**
     * @brief signal emitted when current frame has to be displayed
     */
    void showCurrentFrame();

    /**
     * @brief signal emitted when playing stopped
     */
    void stopped(int selectCellRow = -1, int selectCellColumn = -1);

private slots:

    /**
     * @brief slot called on every timer tick
     */
    void on_playTimerTimeout();

};

#endif // VIDEOPLAYER_H
