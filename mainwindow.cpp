#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QTimer>
#include <QStandardItemModel>
#include <QShortcut>
#include "intervaltimestamp.h"
#include <QTime>
#include <QCloseEvent>
#include <math.h>
#include "navigationeventfilter.h"
#include "tablescripts.h"

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
    sliderFactor = 1;
    stopPlayerDts = 0xffffffffffffffff;

    for(int i = 0; i < IMAGES_BUFFER_SIZE; i++) imagesBuffer[i].image = NULL;

    ui->setupUi(this);

    timeIntervals = new TimeIntervalsModel();

    QDir scritpsPath(QDir::currentPath().append("/scripts"));
    if (!scritpsPath.exists()){
        if (!QDir::current().mkdir("scripts")){
            statusBar()->showMessage(QString("Failed to create scripts directory ").append(scritpsPath.absolutePath()));
        }
    }
    if (scritpsPath.exists()) timeIntervals->tableScripts.Load(scritpsPath);

    NavigationEventFilter *navigationEventFilter = new NavigationEventFilter(this);
    ui->intervalsTableView->installEventFilter(navigationEventFilter);
    ui->intervalsTableView->setModel(timeIntervals);
    ui->intervalsTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    playTimer = new QTimer(this);
    connect(playTimer, SIGNAL(timeout()), this, SLOT(on_playTimerTimeout()));

    QShortcut* openFileShortcut = new QShortcut(QKeySequence(QKeySequence::Open), this);
    connect(openFileShortcut, SIGNAL(activated()), this, SLOT(on_actionOpen_triggered()));
    //ui->actionOpen->setShortcut(openFileShortcut->key());

    QShortcut* saveFileShortcut = new QShortcut(QKeySequence(QKeySequence::Save), this);
    connect(saveFileShortcut, SIGNAL(activated()), this, SLOT(on_actionSave_triggered()));
    //ui->actionSave->setShortcut(saveFileShortcut->key());

    QShortcut* playImageShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(playImageShortcut, SIGNAL(activated()), this, SLOT(on_playPausePushButton_clicked()));
    ui->playPausePushButton->setToolTip(QString("%1 [%2]")
                                        .arg(ui->playPausePushButton->toolTip())
                                        .arg(playImageShortcut->key().toString()));

    QShortcut* playIntervalShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    connect(playIntervalShortcut, SIGNAL(activated()), this, SLOT(on_playIntervalPushButton_clicked()));
    ui->playIntervalPushButton->setToolTip(QString("%1 [%2]")
                                           .arg(ui->playIntervalPushButton->toolTip())
                                           .arg(playIntervalShortcut->key().toString()));

    QShortcut* nextImageShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(nextImageShortcut, SIGNAL(activated()), this, SLOT(on_nextImagePushButton_clicked()));
    ui->nextImagePushButton->setToolTip(QString("%1 [%2]")
                                        .arg(ui->nextImagePushButton->toolTip())
                                        .arg(nextImageShortcut->key().toString()));

    QShortcut* previousImageShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(previousImageShortcut, SIGNAL(activated()), this, SLOT(on_previousImagePushButton_clicked()));
    ui->previousImagePushButton->setToolTip(QString("%1 [%2]")
                                            .arg(ui->previousImagePushButton->toolTip())
                                            .arg(previousImageShortcut->key().toString()));

    QShortcut* reverseJumpShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), this);
    connect(reverseJumpShortcut, SIGNAL(activated()), this, SLOT(on_reverseJumpPushButton_clicked()));
    ui->reverseJumpPushButton->setToolTip(QString("%1 [%2]")
                                          .arg(ui->reverseJumpPushButton->toolTip())
                                          .arg(reverseJumpShortcut->key().toString()));

    QShortcut* forwardJumpShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), this);
    connect(forwardJumpShortcut, SIGNAL(activated()), this, SLOT(on_forwardJumpPushButton_clicked()));
    ui->forwardJumpPushButton->setToolTip(QString("%1 [%2]")
                                          .arg(ui->nextCellPushButton->toolTip())
                                          .arg(forwardJumpShortcut->key().toString()));

    QShortcut* nextTimestampShortcut = new QShortcut(QKeySequence(QKeySequence::InsertParagraphSeparator), this);
    connect(nextTimestampShortcut, SIGNAL(activated()), this, SLOT(on_selectNextCell()));
    ui->nextCellPushButton->setToolTip(QString("%1 [%2]")
                                       .arg(ui->nextCellPushButton->toolTip())
                                       .arg(nextTimestampShortcut->key().toString()));

    QShortcut* deleteTimeIntervalShortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->intervalsTableView);
    connect(deleteTimeIntervalShortcut, SIGNAL(activated()), this, SLOT(on_deletePushButton_clicked()));
    ui->deletePushButton->setToolTip(QString("%1 [%2]")
                                     .arg(ui->deletePushButton->toolTip())
                                     .arg(deleteTimeIntervalShortcut->key().toString()));

    QShortcut* insertTimeIntervalShortcut = new QShortcut(QKeySequence(Qt::Key_Insert), ui->intervalsTableView);
    connect(insertTimeIntervalShortcut, SIGNAL(activated()), this, SLOT(on_insertPushButton_clicked()));
    ui->insertPushButton->setToolTip(QString("%1 [%2]")
                                     .arg(ui->insertPushButton->toolTip())
                                     .arg(insertTimeIntervalShortcut->key().toString()));

    QItemSelectionModel *selectionModel= ui->intervalsTableView->selectionModel();
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(on_selectionChanged(QItemSelection,QItemSelection)));
}

MainWindow::~MainWindow()
{
    delete ui;

    freeDecodingFrameBuffers();
    closeVideoFile();
}

void MainWindow::on_playTimerTimeout(){
    if (!showNextImage() || (imagesBufferCurrent > -1 && imagesBuffer[imagesBufferCurrent].dts >= stopPlayerDts))
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

    // clear state
    closeVideoFile();
    freeDecodingFrameBuffers();
    imagesBufferCurrent = imagesBufferNewest = imagesBufferOldest = -1;
    backSeekFactor = 1;
    sliderFactor = 1;
    stopPlayerDts = 0xffffffffffffffff;
    timeIntervals->clear();
    ui->timeHorizontalSlider->setValue(0);
    statusBar()->showMessage("");

    if (!loadVideoFile(fileName)) return;
    allocateDecodingFrameBuffers();

    if (!opennedVideoFile.isEmpty()){
        timeIntervals->loadIntervals(QString("%1.int").arg(opennedVideoFile));
        setWindowTitle(fileName);
    }

    int64_t streamDuration = av_rescale_q(pFormatCtx->duration, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);
    ui->timeHorizontalSlider->setMaximum(streamDuration / sliderFactor);

    QVariant data = timeIntervals->data(timeIntervals->index(0, 0), Qt::UserRole);
    if (data.isValid()){
        IntervalTimestamp timestamp = data.value<IntervalTimestamp>();
        if (!timestamp.isValid) ui->intervalsTableView->selectionModel()->select(timeIntervals->index(0, 0), QItemSelectionModel::SelectCurrent);
    }

    // current image
    if (readNextFrame()){
        showCurrentImage();
    }
    //next image
    readNextFrame();

    QTime formatDurationTime(0,0,0);
    statusBar()->showMessage(QString("%1 fps, duration: %2")
                             .arg(av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate))
                             .arg(formatDurationTime.addSecs(pFormatCtx->duration * av_q2d(AV_TIME_BASE_Q)).toString("hh:mm:ss.zzz")));
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

void MainWindow::showCurrentImage(bool updateSlider = true){
    if (imagesBufferCurrent != -1 && imagesBuffer[imagesBufferCurrent].image != NULL){
        ui->videoLabel->setImage(imagesBuffer[imagesBufferCurrent].image);

        // update slider
        if (updateSlider){
            double sliderValue = (imagesBuffer[imagesBufferCurrent].dts - pFormatCtx->streams[videoStream]->start_time) / sliderFactor;
            ui->timeHorizontalSlider->setValue(sliderValue);
        }

        // update selected cell in intervals table
        IntervalTimestamp timestamp;
        timestamp.isValid = true;
        timestamp.pts = imagesBuffer[imagesBufferCurrent].pts;
        timestamp.dts = imagesBuffer[imagesBufferCurrent].dts;
        QVariant timestampValue;
        timestampValue.setValue(timestamp);
        foreach(const QModelIndex index, ui->intervalsTableView->selectionModel()->selectedIndexes()){
            timeIntervals->setData(index, timestampValue, Qt::EditRole);
        }
    }
}

bool MainWindow::showNextImage(int jumpImages = 1)
{
    for(int i = 0; i < jumpImages; i++)
        if (imagesBufferCurrent != -1 && imagesBufferCurrent != imagesBufferNewest){
            imagesBufferCurrent = (imagesBufferCurrent + 1) % IMAGES_BUFFER_SIZE;
            if (i == jumpImages - 1) showCurrentImage();
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
    //ui->playPausePushButton->setText(tr("Pause"));
    ui->playPausePushButton->setIcon(QIcon(":/resources/graphics/pause.png"));
    playTimer->start(timeout);
}

void MainWindow::stopPlayer(){
    //ui->playPausePushButton->setText(tr("Play"));
    ui->playPausePushButton->setIcon(QIcon(":/resources/graphics/play.png"));
    playTimer->stop();
    if(stopPlayerDts != 0xffffffffffffffff) ui->intervalsTableView->selectionModel()->select(stopIndex, QItemSelectionModel::SelectCurrent);
    stopPlayerDts = 0xffffffffffffffff;
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

void MainWindow::videoSeek(double targetPts, uint64_t targetDts, bool exactSeek, bool stayOneImageBack){
    //limit backseek factor for case when ffmpeg cannot seek
    while(backSeekFactor < MAX_BACK_SEEK_FACTOR){
        double backSeekDuration = 0;
        if (exactSeek) backSeekDuration = (BACK_SEEK_FRAMES * backSeekFactor) / av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate);
        int64_t seekTarget = (targetPts - backSeekDuration) * AV_TIME_BASE;

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
            if (exactSeek){
                if (stayOneImageBack) while (readNextFrame() && imagesBuffer[imagesBufferNewest].dts < targetDts);
                else while (readNextFrame() && imagesBuffer[imagesBufferNewest].dts <= targetDts);
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

bool MainWindow::showPreviousImage(int jumpImages = 1)
{
    for(int i = 0; i < jumpImages; i++){
        if (imagesBufferCurrent == -1) return false;
        if (imagesBufferCurrent != imagesBufferOldest){
            imagesBufferCurrent = (imagesBufferCurrent - 1 + IMAGES_BUFFER_SIZE) % IMAGES_BUFFER_SIZE;
        }
        else{
            int64_t start_time = av_rescale_q(pFormatCtx->start_time, AV_TIME_BASE_Q, pFormatCtx->streams[videoStream]->time_base);
            if (imagesBuffer[imagesBufferCurrent].dts > start_time){
                videoSeek(imagesBuffer[imagesBufferCurrent].pts, imagesBuffer[imagesBufferCurrent].dts, true, true);
            }
        }
    }
    showCurrentImage();
}

void MainWindow::on_previousImagePushButton_clicked()
{
    stopPlayer();
    showPreviousImage();
}

void MainWindow::on_timeHorizontalSlider_sliderMoved(int position)
{
    if(pFormatCtx == NULL || videoStream == -1) return;

    int64_t streamPosition = (position * sliderFactor) + pFormatCtx->streams[videoStream]->start_time;

    if (position) videoSeek(streamPosition * av_q2d(pFormatCtx->streams[videoStream]->time_base), streamPosition, false, false);
    else videoSeek(streamPosition * av_q2d(pFormatCtx->streams[videoStream]->time_base), streamPosition, true, false);
    showCurrentImage(false);
}

void MainWindow::on_selectNextCell()
{
    if (ui->intervalsTableView->selectionModel()->selectedIndexes().count() <= 0) return;

    QModelIndex index = ui->intervalsTableView->selectionModel()->selectedIndexes()[0];
    if (index.isValid()){
        if (index.row() != timeIntervals->rowCount(index.parent()) - 1){
            // add new row if last editable cell is selected
            if (index.column() == 1 && index.row() == timeIntervals->rowCount(index.parent()) - 2){
                ui->intervalsTableView->model()->insertRows(index.row() + 1, 1, index.parent());
            }
            int newColumn = (index.column() + 1) % 2;
            int newRow = (!newColumn) ? index.row() + 1 : index.row();
            QModelIndex nextIndex = ui->intervalsTableView->model()->index(newRow, newColumn);
            ui->intervalsTableView->selectionModel()->clearSelection();
            ui->intervalsTableView->selectionModel()->select(nextIndex, QItemSelectionModel::SelectCurrent);
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

void MainWindow::on_selectionChanged(const QItemSelection & selected, const QItemSelection & deselected){
    if (selected.count() > 0 && selected.indexes().count() > 0){
        QModelIndex index = selected.indexes().first();
        QVariant data = timeIntervals->data(index, Qt::UserRole);
        IntervalTimestamp timestamp = data.value<IntervalTimestamp>();
        if (timestamp.isValid){
            // jump to selected timestamp
            stopPlayer();
            videoSeek(timestamp.pts, timestamp.dts, true, false);
            showCurrentImage();
        }
        else{
            // fill empty cell with current image timestamp
            if (imagesBufferCurrent > -1){
                IntervalTimestamp currentTimestamp;
                currentTimestamp.dts = imagesBuffer[imagesBufferCurrent].dts;
                currentTimestamp.pts = imagesBuffer[imagesBufferCurrent].pts;
                currentTimestamp.isValid = true;
                QVariant timestampValue;
                timestampValue.setValue(currentTimestamp);
                timeIntervals->setData(index, timestampValue, Qt::EditRole);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveIntervals();

    event->accept();
}

void MainWindow::on_playIntervalPushButton_clicked()
{
    if (ui->intervalsTableView->selectionModel()->selectedIndexes().count() <= 0) return;

    playTimer->stop();
    QModelIndex index = ui->intervalsTableView->selectionModel()->selectedIndexes()[0];
    if (index.isValid()){
        if (index.row() != timeIntervals->rowCount(index.parent()) - 1){
            // add new row if last editable cell is selected
            int newColumn = (index.column() + 1) % 2;
            int newRow = (!newColumn) ? index.row() + 1 : index.row();
            stopIndex = ui->intervalsTableView->model()->index(newRow, newColumn);
            if (stopIndex.isValid()){
                QVariant data = timeIntervals->data(stopIndex, Qt::UserRole);
                if (data.isValid()){
                    IntervalTimestamp timestamp = data.value<IntervalTimestamp>();
                    if (timestamp.isValid){
                        ui->intervalsTableView->selectionModel()->clearSelection();
                        if (pFormatCtx == NULL) return;
                        double timeout = 1 / av_q2d(pFormatCtx->streams[videoStream]->r_frame_rate) * 1000;
                        stopPlayerDts = timestamp.dts;
                        startPlayer(timeout);
                    }
                }
            }
        }
    }
}

void MainWindow::on_insertPushButton_clicked()
{
    QModelIndex idx = ui->intervalsTableView->currentIndex();
    if (idx.isValid()){
        ui->intervalsTableView->model()->insertRows(idx.row(), 1, idx.parent());

        QModelIndex nextIndex = ui->intervalsTableView->model()->index(idx.row(), 0);
        ui->intervalsTableView->setCurrentIndex(nextIndex);
    }
}

void MainWindow::on_deletePushButton_clicked()
{
    QModelIndex idx = ui->intervalsTableView->currentIndex();
    if (idx.isValid())
        ui->intervalsTableView->model()->removeRows(idx.row(), 1, idx.parent());
}

void MainWindow::on_reverseJumpPushButton_clicked()
{
    playTimer->stop();
    showPreviousImage(10);
}

void MainWindow::on_forwardJumpPushButton_clicked()
{
    playTimer->stop();
    showNextImage(10);
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this,
                       "About Video Time Measure",
                       tr(
                           "<h1>Video Time Measure</h1>"
                           "<p>"
                           "This application is intended to measure time in a recorded video.<br />"
                           "Main motivation was to measure and verify air time and synchronization for trampoline."
                           "</p>"
                           "<p>"
                           "Use video from camcoders or hardware encoders. Software encoders can damage time information in video even running on strong hardware."
                           "</p>"
                           "<p>"
                           "To measure time:"
                           "<ol>"
                           "<li>Open video file</li>"
                           "<li>Select appropriate start or stop timestamp in the intervals table right to the video image.</li>"
                           "<li>Navigate to desired video timestamp using buttons below the image.</li>"
                           "</ol>"
                           "Advanced usage for never seen video file:"
                           "<ol>"
                           "<li>Open video file</li>"
                           "<li>Play the video</li>"
                           "<li>Mark desired timestamps by pressing &quot;Enter&quot; during playing video.</li>"
                           "<li>Fine tune created timestamp from the first start timestamp.</li>"
                           "<li>&quot;Play interval&quot; button plays video until next timestamp.</li>"
                           "</ol>"
                           "</p>"
                           "<p>"
                           "Created by Jan Kolorenc</br>"
                           "Source codes are on <a href=\"https://github.com/jankolorenc/VideoTimeMeasure\">GitHub</a><br/>"
                           "Thanks to all people who created libraries, tutorials and tools, this application is based on. (see source code)"
                           "</p>"
                           ));
}
