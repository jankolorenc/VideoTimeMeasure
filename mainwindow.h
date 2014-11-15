#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QItemSelection>
#include "videoimage.h"
#include "timeintervalsmodel.h"
#include "videoparametersmodel.h"

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

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    QTimer *playTimer;

    TimeIntervalsModel *timeIntervals;
    VideoParametersModel *videoParameters;
    QString opennedVideoFile;

    AVDictionary **options;
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    int videoStream;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    uint8_t *buffer;
    struct SwsContext *sws_ctx;
    double video_clock;
    int backSeekFactor;
    double pts;
    int sliderFactor;

    VideoImage imagesBuffer[IMAGES_BUFFER_SIZE];
    int imagesBufferOldest;
    int imagesBufferNewest;
    int imagesBufferCurrent;

    uint64_t stopPlayerDts;
    QModelIndex stopIndex;

    void closeEvent(QCloseEvent *event);

    void showError(QString text);
    bool loadVideoFile(QString filename);
    void closeVideoFile();
    void allocateDecodingFrameBuffers();
    void freeDecodingFrameBuffers();
    bool readNextFrame();
    void bufferCurrentFrame();
    void showCurrentImage(bool updateSlider = true);
    double synchronizeVideo(AVFrame *src_frame, double pts);
    void videoSeek(double targetPts, uint64_t targetDts, bool exactSeek, bool stayOneImageBack);
    bool showNextImage(int jumpImages = 1);
    bool showPreviousImage(int jumpImages = 1);
    void saveIntervals();
    void startPlayer(double timeout);
    void stopPlayer();

private slots:
    void on_actionOpen_triggered();
    void on_playPausePushButton_clicked();
    void on_playTimerTimeout();
    void on_previousImagePushButton_clicked();
    void on_nextImagePushButton_clicked();
    void on_timeHorizontalSlider_sliderMoved(int position);
    void on_deleteIntervalRow();
    void on_insertIntervalRow();
    void on_actionSave_triggered();
    void on_selectionChanged(const QItemSelection &, const QItemSelection &);
    void on_previousJumpPushButton_clicked();
    void on_nextJumpPushButton_clicked();
    void on_selectNextCell();
    void on_playTillNextTimestampPushButton_clicked();
};

#endif // MAINWINDOW_H
