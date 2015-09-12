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
#include "scripteditor.h"
#include "newscriptprofileform.h"
#include "tablelimits.h"
#include <boost/filesystem.hpp>
#include <QDirIterator>
#include <QDebug>

Q_DECLARE_METATYPE(IntervalTimestamp)

namespace fs = boost::filesystem;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scriptProfilesActionGroup = new QActionGroup(this);

    timeIntervals = new TimeIntervalsModel();

    NavigationEventFilter *navigationEventFilter = new NavigationEventFilter(this);
    ui->intervalsTableView->installEventFilter(navigationEventFilter);
    ui->intervalsTableView->setModel(timeIntervals);
    ui->intervalsTableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    connect(&videoPlayer, SIGNAL(showCurrentFrame()), this, SLOT(showCurrentFrame()));
    connect(&videoPlayer, SIGNAL(stopped(int,int)), this, SLOT(videoPlayerStopped(int,int)));

    QShortcut* openFileShortcut = new QShortcut(QKeySequence(QKeySequence::Open), this);
    connect(openFileShortcut, SIGNAL(activated()), this, SLOT(on_actionOpen_triggered()));

    QShortcut* saveFileShortcut = new QShortcut(QKeySequence(QKeySequence::Save), this);
    connect(saveFileShortcut, SIGNAL(activated()), this, SLOT(on_actionSave_triggered()));

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

    connect(ui->intervalsTableView, SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(on_tableContextMenuRequested(QPoint)));

    ui->intervalsTableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->intervalsTableView->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(on_horizontalHeaderContextMenuRequested(QPoint)));

    ui->intervalsTableView->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->intervalsTableView->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(on_verticalHeaderContextMenuRequested(QPoint)));

    fillScriptProfiles();

    session.load();

}

QAction* MainWindow::registerScriptProfile(QString name){
    if (name == DEFAULT_PROFILE) return NULL;

    // check existing profile
    QList<QAction *> actions = scriptProfilesActionGroup->actions();
    for (int i = 0; i < actions.length(); i++) {
        if (actions[i]->text() == name) return actions[i];
    }
    {
        QAction *action = ui->menuScriptProfiles->addAction(name);
        action->setCheckable(TRUE);
        connect(action, SIGNAL(triggered(bool)), SLOT(on_actionProfile_triggered(bool)));
        scriptProfilesActionGroup->addAction(action);
        return action;
    }
}

void MainWindow::fillScriptProfiles(QString scriptsDir){
    QDir scritpsPath(scriptsDir);
    if (!scritpsPath.exists()) return;

    foreach (QFileInfo dirInfo, scritpsPath.entryInfoList(QDir::Dirs|QDir::NoSymLinks|QDir::NoDotAndDotDot , QDir::Unsorted)) {
        registerScriptProfile(dirInfo.baseName());
    }
}

void MainWindow::fillScriptProfiles(){
    fillScriptProfiles(timeIntervals->scriptsDirectory());
    timeIntervals->loadScriptProfile(timeIntervals->scriptsProfile(), timeIntervals->scriptsDirectory());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showError(QString text){
    QMessageBox msgBox;
    msgBox.setText(text);
    msgBox.exec();
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), session.lastVideoDirectory(), tr("Files (*.*)"));
    if (fileName.isEmpty()) return;

    videoPlayer.clearState();
    saveIntervals();

    session.setOpennedVideo("");
    timeIntervals->clear();
    ui->timeHorizontalSlider->setValue(0);
    statusBar()->showMessage("");

    if (!videoPlayer.loadFile(fileName)){
        showError("Invalid video");
        return;
    }
    session.setOpennedVideo(fileName);
    videoLoaded = true;

    if (!session.opennedVideo().isEmpty()){
        timeIntervals->loadIntervals(QString("%1.int").arg(session.opennedVideo()));
        setWindowTitle(fileName);
    }

    int64_t streamDuration = videoPlayer.streamDuration();
    ui->timeHorizontalSlider->setMaximum(streamDuration / videoPlayer.sliderFactor);

    QVariant data = timeIntervals->data(timeIntervals->index(0, 0), Qt::UserRole);
    if (data.isValid()){
        IntervalTimestamp timestamp = data.value<IntervalTimestamp>();
        if (!timestamp.isValid) ui->intervalsTableView->selectionModel()->select(timeIntervals->index(0, 0), QItemSelectionModel::SelectCurrent);
    }

    // current image
    if (videoPlayer.readNextFrame()){
        showCurrentFrame();
    }
    //next image
    videoPlayer.readNextFrame();

    QTime formatDurationTime(0,0,0);
    statusBar()->showMessage(QString("%1 fps, duration: %2")
                             .arg(videoPlayer.framerate())
                             .arg(formatDurationTime.addSecs(videoPlayer.durationSeconds()).toString("hh:mm:ss.zzz")));
}

void MainWindow::showCurrentFrame(bool updateSlider){
    VideoImage *currentImage = videoPlayer.currentImage();
    if (currentImage != NULL){
        ui->videoLabel->setImage(currentImage->image);

        // update slider
        if (updateSlider){
            double sliderValue = (currentImage->dts - videoPlayer.startTime()) / videoPlayer.sliderFactor;
            ui->timeHorizontalSlider->setValue(sliderValue);
        }

        // update selected cell in intervals table
        IntervalTimestamp timestamp;
        timestamp.isValid = true;
        timestamp.pts = currentImage->pts;
        timestamp.dts = currentImage->dts;
        QVariant timestampValue;
        timestampValue.setValue(timestamp);
        foreach(const QModelIndex index, ui->intervalsTableView->selectionModel()->selectedIndexes()){
            timeIntervals->setData(index, timestampValue, Qt::EditRole);
        }
    }
}

void MainWindow::on_nextImagePushButton_clicked()
{
    stopPlayer();
    videoPlayer.stepForward();
}

void MainWindow::startPlayer(IntervalTimestamp *stop, int selectCellRow, int selectCellColumn){
    ui->playPausePushButton->setIcon(QIcon(":/resources/graphics/pause.png"));
    videoPlayer.play(stop, selectCellRow, selectCellColumn);
}

void MainWindow::stopPlayer(){
    ui->playPausePushButton->setIcon(QIcon(":/resources/graphics/play.png"));
    videoPlayer.stop();
}

void MainWindow::on_playPausePushButton_clicked()
{
    if (videoPlayer.isEmpty()) return;
    if (!videoPlayer.isPlaying())
        startPlayer();
    else
        stopPlayer();
}

void MainWindow::on_previousImagePushButton_clicked()
{
    stopPlayer();
    videoPlayer.stepReverse();
}

void MainWindow::on_timeHorizontalSlider_sliderMoved(int position)
{
    if(videoPlayer.isEmpty()) return;

    int64_t streamPosition = (position * videoPlayer.sliderFactor) + videoPlayer.startTime();

    if (position) videoPlayer.seek(streamPosition * videoPlayer.timebase(), streamPosition, false, false);
    else videoPlayer.seek(streamPosition * videoPlayer.timebase(), streamPosition, true, false);
    showCurrentFrame(false);
}

void MainWindow::on_selectNextCell()
{
    if (ui->intervalsTableView->selectionModel()->selectedIndexes().count() <= 0) return;

    QModelIndex index = ui->intervalsTableView->selectionModel()->selectedIndexes()[0];
    if (index.isValid()){
        if (index.row() != timeIntervals->rowCount(index.parent()) - 1){
            // add new row if last editable cell is selected
            if (index.column() == 1 && index.row() == timeIntervals->intervalsCount() - 1){
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
    if (videoLoaded && !session.opennedVideo().isEmpty()) timeIntervals->saveIntervals(QString("%1.int").arg(session.opennedVideo()));
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
            videoPlayer.seek(timestamp.pts, timestamp.dts, true, false);
            showCurrentFrame();
        }
        else{
            // fill empty cell with current image timestamp
            VideoImage *currentImage = videoPlayer.currentImage();
            if (currentImage != NULL){
                IntervalTimestamp currentTimestamp;
                currentTimestamp.dts = currentImage->dts;
                currentTimestamp.pts = currentImage->pts;
                currentTimestamp.isValid = true;
                QVariant timestampValue;
                timestampValue.setValue(currentTimestamp);
                timeIntervals->setData(index, timestampValue, Qt::EditRole);
            }
        }
    }
}

void MainWindow::on_tableContextMenuRequested(QPoint position){
    if (!timeIntervals->editingTableScripts) return;
    QModelIndex index = ui->intervalsTableView->indexAt(position);
    editScriptRow = index.row();
    editScriptColumn = index.column();

    QMenu *menu = new QMenu(this);
    QAction *action = new QAction("Add column", this);
    connect(action, SIGNAL(triggered()), SLOT(on_addNewScriptColumn_triggered()));
    menu->addAction(action);
    action = new QAction("Add row", this);
    connect(action, SIGNAL(triggered()), SLOT(on_addNewScriptRow_triggered()));
    menu->addAction(action);

    if (index.column() >= FIXED_COLUMS){
        action = new QAction("Remove column", this);
        connect(action, SIGNAL(triggered()), SLOT(on_removeScriptColumn_triggered()));
        menu->addAction(action);
    }
    if (index.row() > timeIntervals->intervalsCount()){
        action = new QAction("Remove row", this);
        connect(action, SIGNAL(triggered()), SLOT(on_removeScriptRow_triggered()));
        menu->addAction(action);
        action = new QAction("Edit cell script", this);
        connect(action, SIGNAL(triggered()), SLOT(on_editScript()));
        menu->addAction(action);
    }

    menu->popup(ui->intervalsTableView->viewport()->mapToGlobal(position));
}

void MainWindow::on_horizontalHeaderContextMenuRequested(QPoint position){
    if (!timeIntervals->editingTableScripts) return;

    editScriptColumn = ui->intervalsTableView->horizontalHeader()->logicalIndexAt(position);
    editScriptRow = -1;
    QMenu *menu = new QMenu(this);
    QAction *action = new QAction("Add column", this);
    connect(action, SIGNAL(triggered()), SLOT(on_addNewScriptColumn_triggered()));
    menu->addAction(action);
    if (editScriptColumn >= FIXED_COLUMS){
        action = new QAction("Remove column", this);
        connect(action, SIGNAL(triggered()), SLOT(on_removeScriptColumn_triggered()));
        menu->addAction(action);
    }
    action = new QAction("Edit column script", this);
    menu->addAction(action);
    connect(action, SIGNAL(triggered()), SLOT(on_editScript()));

    menu->popup(ui->intervalsTableView->viewport()->mapToGlobal(position));
}

void MainWindow::on_verticalHeaderContextMenuRequested(QPoint position){
    if (!timeIntervals->editingTableScripts) return;

    editScriptRow = ui->intervalsTableView->verticalHeader()->logicalIndexAt(position);
    editScriptColumn = -1;
    QMenu *menu = new QMenu(this);
    QAction *action;
    action = new QAction("Add row", this);
    connect(action, SIGNAL(triggered()), SLOT(on_addNewScriptRow_triggered()));
    menu->addAction(action);
    if (editScriptRow > timeIntervals->intervalsCount()){
        action = new QAction("Remove row", this);
        connect(action, SIGNAL(triggered()), SLOT(on_removeScriptRow_triggered()));
        menu->addAction(action);
        action = new QAction("Edit row script", this);
        menu->addAction(action);
        connect(action, SIGNAL(triggered()), SLOT(on_editScript()));
    }

    menu->popup(ui->intervalsTableView->viewport()->mapToGlobal(position));
}

void MainWindow::on_addNewScriptColumn_triggered(){
    int column = (editScriptColumn < FIXED_COLUMS - 1) ? timeIntervals->columnCount() - 1: editScriptColumn;
    timeIntervals->insertColumn(column + 1); // +1 = add instead of insert
}


void MainWindow::on_addNewScriptRow_triggered(){
    int row = (editScriptRow < timeIntervals->intervalsCount()) ? timeIntervals->rowCount() - 1 : editScriptRow;
    timeIntervals->insertRow(row + 1); // +1 = add instead of insert
}

void MainWindow::on_removeScriptColumn_triggered(){
    int column = (editScriptColumn < FIXED_COLUMS) ? timeIntervals->columnCount() - 1 : editScriptColumn;
    timeIntervals->removeColumn(column);
}


void MainWindow::on_removeScriptRow_triggered(){
    int row = (editScriptRow < timeIntervals->intervalsCount()) ? timeIntervals->rowCount() - 1 : editScriptRow;
    timeIntervals->removeRow(row);
}

void MainWindow::on_editScript(){
    ScriptEditor editor(this);
    editor.setScript(timeIntervals->getScript(editScriptRow, editScriptColumn));
    if (editor.exec() == QDialog::Accepted){
        timeIntervals->setScript(editScriptRow, editScriptColumn, editor.getScript());
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

    videoPlayer.stop();
    QModelIndex index = ui->intervalsTableView->selectionModel()->selectedIndexes()[0];
    if (index.isValid()){
        if (index.row() != timeIntervals->rowCount(index.parent()) - 1){
            // add new row if last editable cell is selected
            int newColumn = (index.column() + 1) % 2;
            int newRow = (!newColumn) ? index.row() + 1 : index.row();
            QModelIndex stopIndex = index.sibling(newRow, newColumn);
            if (stopIndex.isValid()){
                QVariant data = timeIntervals->data(stopIndex, Qt::UserRole);
                if (data.isValid()){
                    IntervalTimestamp timestamp = data.value<IntervalTimestamp>();
                    if (timestamp.isValid){
                        ui->intervalsTableView->selectionModel()->clearSelection();
                        if (videoPlayer.isEmpty()) return;
                        startPlayer(&timestamp, stopIndex.row(), stopIndex.column());
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
    stopPlayer();
    videoPlayer.stepReverse(10);
}

void MainWindow::on_forwardJumpPushButton_clicked()
{
    stopPlayer();
    videoPlayer.stepForward(10);
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


void MainWindow::on_action_Clear_triggered()
{
    foreach (QAction *action, scriptProfilesActionGroup->actions()) {
        if (action->isChecked()) action->setChecked(false);
    }
    ui->actionDelete->setEnabled(false);
    timeIntervals->loadScriptProfile(DEFAULT_PROFILE, timeIntervals->scriptsDirectory());
    timeIntervals->deleteScriptProfile(DEFAULT_PROFILE);
}

void MainWindow::on_actionProfile_triggered(bool checked)
{
    if (checked){
        QAction *action = (QAction *)QObject::sender();
        timeIntervals->loadScriptProfile(action->text(), timeIntervals->scriptsDirectory());
        ui->actionDelete->setEnabled(true);
    }
}

void MainWindow::on_actionNew_triggered()
{
    NewScriptProfileForm profileForm(this);
    if (profileForm.exec() == QDialog::Accepted){
        QString newProfileName = profileForm.getProfileName();
        if (fs::native(newProfileName.toUtf8().constData())){
            if (!(newProfileName.isNull() || newProfileName.isEmpty())){
                timeIntervals->saveScriptProfile(newProfileName);
                ui->actionDelete->setEnabled(true);
                timeIntervals->loadScriptProfile(newProfileName, timeIntervals->scriptsDirectory());
                QAction *action = registerScriptProfile(newProfileName);
                if (action != NULL) action->setCheckable(TRUE);
            }
        }
        else showError("Invalid profile name");
    }
}

void MainWindow::on_actionEdit_changed()
{
    timeIntervals->editingTableScripts = ui->actionEdit->isChecked();
    ui->intervalsTableView->reset(); // trying to repaint table (no better method found)
}

void MainWindow::on_actionDelete_triggered()
{
    foreach (QAction *action, scriptProfilesActionGroup->actions()) {
        if (action->isChecked()){
            timeIntervals->deleteScriptProfile(action->text());
            scriptProfilesActionGroup->removeAction(action);
            ui->menuScriptProfiles->removeAction(action);
        }
    }
    ui->actionDelete->setEnabled(false);
}

void MainWindow::on_intervalsTableView_doubleClicked(const QModelIndex &index)
{
    if (timeIntervals->editingTableScripts){
        if (index.row() > timeIntervals->intervalsCount()){
            editScriptRow = index.row();
            editScriptColumn = index.column();
            on_editScript();
        }
    }
}

void MainWindow::videoPlayerStopped(int selectCellRow, int selectCellColumn){
    if (selectCellRow > -1 && selectCellColumn > -1){
        ui->intervalsTableView->selectionModel()->select(
                    ui->intervalsTableView->model()->index(selectCellRow, selectCellColumn),
                    QItemSelectionModel::SelectCurrent);
    }
}

void MainWindow::on_nextCellPushButton_clicked()
{
    on_selectNextCell();
}

// copy script examples to user directory
void MainWindow::on_action_Get_examples_triggered()
{
    QDirIterator profilesIterator(":/scripts");
    while (profilesIterator.hasNext()) {
        profilesIterator.next();

        //create destination directory
        QDir destinationDirectory(timeIntervals->scriptsDirectory() + profilesIterator.fileName());
        if (destinationDirectory.exists()){
            // remove existing scripts
            QRegExp regex("(col|row)-(\\d+)");
            foreach (QString fileName, destinationDirectory.entryList(QStringList("*.js"), QDir::Files|QDir::Readable, QDir::Unsorted)){
                if (regex.indexIn(fileName) != -1) QFile::remove(destinationDirectory.absoluteFilePath(fileName));
            }
        }
        else destinationDirectory.mkpath(".");

        QDirIterator scriptsIterator(":/scripts/" + profilesIterator.fileName());
        while (scriptsIterator.hasNext()) {
            scriptsIterator.next();
            QFile file(scriptsIterator.fileInfo().absoluteFilePath());
            file.copy(destinationDirectory.absolutePath() + "/" + scriptsIterator.fileName());
        }

        QAction *action = registerScriptProfile(profilesIterator.fileName());
        if (action != NULL) action->setCheckable(TRUE);
    }
}
