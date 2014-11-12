#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QTimer>
#include <QStandardItemModel>
#include <QShortcut>
#include "intervaltimestamp.h"
#include "videoparametersmodel.h"
#include <QTime>
#include <QCloseEvent>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

Q_DECLARE_METATYPE(IntervalTimestamp)

uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

int our_get_buffer(struct AVCodecContext *c, AVFrame *pic) {
    int ret = avcodec_default_get_buffer(c, pic);
    uint64_t *pts = av_malloc(sizeof(uint64_t));
    *pts = global_video_pkt_pts;
    pic->opaque = pts;
    return ret;
}

void our_release_buffer(struct AVCodecContext *c, AVFrame *pic) {
    if(pic) av_freep(&pic->opaque);
    avcodec_default_release_buffer(c, pic);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
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

    for(int i = 0; i < IMAGES_BUFFER_SIZE; i++) imagesBuffer[i].image = NULL;

    ui->setupUi(this);

    playTimer = new QTimer(this);
    connect(playTimer, SIGNAL(timeout()), this, SLOT(on_playTimerTimeout()));

    timeIntervals = new TimeIntervalsModel();

    ui->intervalsTableView->setModel(timeIntervals);
    ui->intervalsTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    QShortcut* openFileShortcut = new QShortcut(QKeySequence(QKeySequence::Open), this);
    connect(openFileShortcut, SIGNAL(activated()), this, SLOT(on_actionOpen_triggered()));
    QShortcut* saveFileShortcut = new QShortcut(QKeySequence(QKeySequence::Save), this);
    connect(saveFileShortcut, SIGNAL(activated()), this, SLOT(on_actionSave_triggered()));
    QShortcut* playImageShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(playImageShortcut, SIGNAL(activated()), this, SLOT(on_playPausePushButton_clicked()));
    QShortcut* deleteTimeIntervalShortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->intervalsTableView);
    connect(deleteTimeIntervalShortcut, SIGNAL(activated()), this, SLOT(on_deleteIntervalRow()));
    QShortcut* addTimeIntervalShortcut = new QShortcut(QKeySequence(QKeySequence::InsertParagraphSeparator), ui->intervalsTableView);
    connect(addTimeIntervalShortcut, SIGNAL(activated()), this, SLOT(on_addIntervalRow()));
    QShortcut* nextImageShortcut = new QShortcut(QKeySequence(Qt::Key_Plus), this);
    connect(nextImageShortcut, SIGNAL(activated()), this, SLOT(on_nextImagePushButton_clicked()));
    QShortcut* previousImageShortcut = new QShortcut(QKeySequence(Qt::Key_Minus), this);
    connect(previousImageShortcut, SIGNAL(activated()), this, SLOT(on_previousImagePushButton_clicked()));

    QItemSelectionModel *selectionModel= ui->intervalsTableView->selectionModel();
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(on_selectionChanged(QItemSelection,QItemSelection)));

    videoParameters = new VideoParametersModel();
    ui->videoParametersTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
}

MainWindow::~MainWindow()
{
    delete ui;

    freeDecodingFrameBuffers();
    closeVideoFile();
}

void MainWindow::on_playTimerTimeout(){
    if (!showNextImage())
        stopPlayer();
}

void MainWindow::showError(QString text){
    QMessageBox msgBox;
    msgBox.setText(text);
    //msgBox.Icon = QMessageBox::Critical;
    msgBox.exec();
}

bool MainWindow::loadVideoFile(QString fileName){
    // Open video file
    QByteArray fileNameByteArray = fileName.toLocal8Bit();
    //if(int result = avformat_open_input(&pFormatCtx,filename, NULL, options)!=0){
    int result = avformat_open_input(&pFormatCtx,fileNameByteArray.data(), NULL, options);
    if(result < 0){
        char error_string[200];
        av_strerror(result, error_string, 200);
        showError(tr("Couldn't open file %1: %2").arg(fileName).arg(QString(error_string)));
        return FALSE;
    }

    if(avformat_find_stream_info(pFormatCtx, options)<0){
        showError(tr("Couldn't find stream information in video"));
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
        showError(tr("Didn't find a video stream"));
        return FALSE;
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        showError(tr("Unsupported codec"));
        return FALSE;
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, options)<0){
        showError(tr("Could not open codec"));
        return FALSE;
    }

    pCodecCtx->get_buffer = our_get_buffer;
    pCodecCtx->release_buffer = our_release_buffer;

    opennedVideoFile = fileName;

    return TRUE;
}

void MainWindow::closeVideoFile(){    
    saveIntervals();

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

    opennedVideoFile.clear();
}

void MainWindow::allocateDecodingFrameBuffers(){

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
        showError(tr("Couldn't not allocate frame"));
        return;
    }

    // Allocate an AVFrame structure
    pFrameRGB=avcodec_alloc_frame();
    if(pFrameRGB==NULL){
        showError(tr("Couldn't not allocate RGB frame"));
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

void MainWindow::freeDecodingFrameBuffers(){
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

double MainWindow::synchronizeVideo(AVFrame *src_frame, double pts) {
    double frame_delay;
    if(pts != 0) {
        /* if we have pts, set video clock to it */
        video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(pFormatCtx->streams[videoStream]->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    video_clock += frame_delay;
    return pts;
}

bool MainWindow::readNextFrame(){
    if (pFormatCtx == NULL) return FALSE;

    AVPacket packet;
    int frameFinished = 0;

    while(av_read_frame(pFormatCtx, &packet)>=0) {
        if(packet.stream_index==videoStream) {
            pts = 0;
            // Save global pts to be stored in pFrame in first call
            global_video_pkt_pts = packet.pts;
            // Is this a packet from the video stream?

            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if(packet.dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
                pts = *(uint64_t *)pFrame->opaque;
            } else if(packet.dts != AV_NOPTS_VALUE) {
                pts = packet.dts;
            } else {
                pts = 0;
            }
            pts *= av_q2d(pFormatCtx->streams[videoStream]->time_base);
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

QString lastDirectory;
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), lastDirectory, tr("Files (*.*)"));
    if (fileName.isEmpty()) return;

    lastDirectory = QFileInfo(fileName).absoluteDir().absolutePath();

    closeVideoFile();
    freeDecodingFrameBuffers();
    imagesBufferCurrent = imagesBufferNewest = imagesBufferOldest = -1;
    backSeekFactor = 1;

    videoParameters->parameters.clear();

    ui->timeHorizontalSlider->setValue(0);
    if (!loadVideoFile(fileName)) return;
    allocateDecodingFrameBuffers();

    if (!opennedVideoFile.isEmpty()) timeIntervals->loadIntervals(QString("%1.int").arg(opennedVideoFile));

    // current image
    if (readNextFrame()){
        firstImagePts = imagesBuffer[imagesBufferNewest].pts;
        firstImageDts = imagesBuffer[imagesBufferNewest].dts;
        showCurrentImage();
    }
    //next image
    readNextFrame();

    //fill video info parameters - for debugging and visual check
    QPair<QString, QString> avgFramerate("avg framerate", QString("%1 / %2 = %3 fps")
                                         .arg(pFormatCtx->streams[videoStream]->avg_frame_rate.num)
                                         .arg(pFormatCtx->streams[videoStream]->avg_frame_rate.den)
                                         .arg(av_q2d(pFormatCtx->streams[videoStream]->avg_frame_rate))
                                         );
    videoParameters->parameters.append(avgFramerate);
    QPair<QString, QString> rFramerate("r framerate", QString("%1 / %2 = %3 fps")
                                       .arg(pFormatCtx->streams[videoStream]->r_frame_rate.num)
                                       .arg(pFormatCtx->streams[videoStream]->r_frame_rate.den)
                                       .arg(av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate))
                                       );
    videoParameters->parameters.append(rFramerate);
    QTime streamDurationTime(0,0,0);
    QPair<QString, QString> streamDuration("stream duration", QString("%1 = %2 s")
                                           .arg(pFormatCtx->streams[videoStream]->duration)
                                           .arg(streamDurationTime.addSecs(pFormatCtx->streams[videoStream]->duration * av_q2d(pFormatCtx->streams[videoStream]->time_base)).toString("hh:mm:ss.zzz")));
    videoParameters->parameters.append(streamDuration);

    QTime formatDurationTime(0,0,0);
    QPair<QString, QString> formatDuration("format duration", QString("%1 = %2 s")
                                           .arg(pFormatCtx->duration)
                                           .arg(formatDurationTime.addSecs(pFormatCtx->duration * av_q2d(AV_TIME_BASE_Q)).toString("hh:mm:ss.zzz")));
    videoParameters->parameters.append(formatDuration);

    QPair<QString, QString> gop("gop", QString("%1")
                                .arg(pFormatCtx->streams[videoStream]->codec->gop_size)
                                );
    videoParameters->parameters.append(gop);

    ui->videoParametersTableView->setModel(videoParameters);
}

void MainWindow::bufferCurrentFrame(){
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

void MainWindow::showCurrentImage(){
    if (imagesBufferCurrent != -1 && imagesBuffer[imagesBufferCurrent].image != NULL){
        ui->videoLabel->setPixmap(QPixmap::fromImage(
                                      imagesBuffer[imagesBufferCurrent].image->scaled(ui->videoLabel->width(),
                                                                                      ui->videoLabel->height(),
                                                                                      Qt::KeepAspectRatio)));
        ui->timeLabel->setText(QString("PTS %1, DTS %2")
                               .arg(imagesBuffer[imagesBufferCurrent].pts)
                               .arg(imagesBuffer[imagesBufferCurrent].dts));

        //double contextPositionDelta = imagesBuffer[imagesBufferCurrent].pts - firstImagePts;
        double contextPositionDelta = imagesBuffer[imagesBufferCurrent].pts;
        double durationSeconds = pFormatCtx->duration * av_q2d(AV_TIME_BASE_Q);
        double sliderValue = (contextPositionDelta / durationSeconds) * ui->timeHorizontalSlider->maximum();
        ui->timeHorizontalSlider->setValue(round(sliderValue));

        IntervalTimestamp timestamp;
        timestamp.isValid = true;
        timestamp.pts = imagesBuffer[imagesBufferCurrent].pts;
        timestamp.dts = imagesBuffer[imagesBufferCurrent].dts;
        QVariant timestampValue;
        timestampValue.setValue(timestamp);
        timeIntervals->setData(ui->intervalsTableView->currentIndex(), timestampValue, Qt::EditRole);
    }
}

bool MainWindow::showNextImage()
{
    if (imagesBufferCurrent != -1 && imagesBufferCurrent != imagesBufferNewest){
        imagesBufferCurrent = (imagesBufferCurrent + 1) % IMAGES_BUFFER_SIZE;
        showCurrentImage();
        if (imagesBufferCurrent == imagesBufferNewest) readNextFrame();
    }
    else return false;

    return true;
}

void MainWindow::on_nextImagePushButton_clicked()
{
    stopPlayer();
    showNextImage();
}

void MainWindow::startPlayer(double timeout){
    ui->playPausePushButton->setText(tr("Pause"));
    playTimer->start(timeout);
}

void MainWindow::stopPlayer(){
    ui->playPausePushButton->setText(tr("Play"));
    playTimer->stop();
}

void MainWindow::on_playPausePushButton_clicked()
{
    if (pFormatCtx == NULL) return;

    double timeout = 1 / av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate) * 1000;

    if (!playTimer->isActive())
        startPlayer(timeout);
    else
        stopPlayer();
}

void MainWindow::videoFrameSeek(double targetPts, uint64_t targetDts, bool stopOnPreviousFrame = false){

    //limit backseek factor for case when ffmpeg cannot seek
    while(backSeekFactor < MAX_BACK_SEEK_FACTOR){
        double seekDuration = (BACK_SEEK_FRAMES * backSeekFactor) / av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate);
        int64_t seekTarget = (targetPts - seekDuration) * AV_TIME_BASE;

        // hope this is fix for mjpeg back seek causing crash
        if (pFormatCtx->start_time >=0 && seekTarget < 0) seekTarget = 0;

        seekTarget = av_rescale_q(seekTarget, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);

        int result = av_seek_frame(pFormatCtx, videoStream, seekTarget, AVSEEK_FLAG_BACKWARD);
        if (result >= 0){
            avcodec_flush_buffers(pFormatCtx->streams[videoStream]->codec);
            // flush imagesBuffer
            imagesBufferCurrent = imagesBufferNewest = imagesBufferOldest = -1;
            video_clock = 0;

            // read and buffer previous images
            if (stopOnPreviousFrame) while (readNextFrame() && imagesBuffer[imagesBufferNewest].dts < targetDts);
            else while (readNextFrame() && imagesBuffer[imagesBufferNewest].dts <= targetDts);

            if (imagesBufferNewest != imagesBufferOldest){
                imagesBufferCurrent = (imagesBufferNewest -1 + IMAGES_BUFFER_SIZE) % IMAGES_BUFFER_SIZE;
                break;
            }
            else{
                backSeekFactor++;
            }
        }
    }

    // restart backseek factor to allow seek to another target
    if (backSeekFactor >= MAX_BACK_SEEK_FACTOR) backSeekFactor = 1;
}

void MainWindow::on_previousImagePushButton_clicked()
{
    if (imagesBufferCurrent == -1) return;

    stopPlayer();

    if (imagesBufferCurrent != imagesBufferOldest){
        imagesBufferCurrent = (imagesBufferCurrent - 1 + IMAGES_BUFFER_SIZE) % IMAGES_BUFFER_SIZE;
        showCurrentImage();
    }
    else{
        if (imagesBuffer[imagesBufferCurrent].dts > firstImageDts){
            videoFrameSeek(imagesBuffer[imagesBufferCurrent].pts, imagesBuffer[imagesBufferCurrent].dts, true);
            showCurrentImage();
        }
    }
}

void MainWindow::on_timeHorizontalSlider_sliderMoved(int position)
{
    if(pFormatCtx == NULL || videoStream == -1) return;

    double contextPositionDelta = pFormatCtx->duration * (double)position / (double)ui->timeHorizontalSlider->maximum();
    int64_t target = av_rescale_q(contextPositionDelta, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);

    if (av_seek_frame(pFormatCtx, videoStream, target, AVSEEK_FLAG_BACKWARD) < 0){
        //error
    }
    else{
        avcodec_flush_buffers(pFormatCtx->streams[videoStream]->codec);
        // flush imagesBuffer
        imagesBufferCurrent = imagesBufferNewest = imagesBufferOldest = -1;
        video_clock = 0;
        if (readNextFrame()){
            showCurrentImage();
        }
        //next image
        readNextFrame();
    }
}

void MainWindow::on_deleteIntervalRow()
{
    QModelIndex idx = ui->intervalsTableView->currentIndex();
    if (idx.isValid())
        ui->intervalsTableView->model()->removeRows(idx.row(), 1, idx.parent());
}

void MainWindow::on_addIntervalRow()
{
    QModelIndex idx = ui->intervalsTableView->currentIndex();
    if (idx.isValid()){
        if (idx.row() == timeIntervals->rowCount(idx.parent()) - 1 ||
                (idx.row() == 0 && idx.column() == 0)){
            ui->intervalsTableView->model()->insertRows(idx.row(), 1, idx.parent());

            QModelIndex nextIndex = ui->intervalsTableView->model()->index(idx.row(), 0);
            ui->intervalsTableView->setCurrentIndex(nextIndex);
        }
        else{
            ui->intervalsTableView->model()->insertRows(idx.row() + 1, 1, idx.parent());
            QModelIndex nextIndex = ui->intervalsTableView->model()->index(idx.row() + 1, 0);
            ui->intervalsTableView->setCurrentIndex(nextIndex);
        }
    }
}

void MainWindow::saveIntervals(){
    if (!opennedVideoFile.isEmpty()) timeIntervals->saveIntervals(QString("%1.int").arg(opennedVideoFile));
}

void MainWindow::on_actionSave_triggered()
{
    saveIntervals();
}

void MainWindow::on_selectionChanged(const QItemSelection &, const QItemSelection &){
    if (ui->intervalsTableView->selectionModel()->selectedIndexes().count() > 0){
        QVariant data = timeIntervals->data(ui->intervalsTableView->selectionModel()->selectedIndexes()[0], Qt::UserRole);
        IntervalTimestamp timestamp = data.value<IntervalTimestamp>();
        if (timestamp.isValid){
            stopPlayer();
            videoFrameSeek(timestamp.pts, timestamp.dts);
            showCurrentImage();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveIntervals();

    event->accept();
}
