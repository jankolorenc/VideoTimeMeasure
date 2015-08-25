#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemSelection>
#include <QDir>
#include <QActionGroup>
#include "videoimage.h"
#include "timeintervalsmodel.h"
#include "tablescripts.h"
#include "videoplayer.h"

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
    int editScriptColumn;
    int editScriptRow;
    QString opennedVideoFile;

    Ui::MainWindow *ui;

    VideoPlayer videoPlayer;

    TimeIntervalsModel *timeIntervals;

    QActionGroup *scriptProfilesActionGroup;

    void closeEvent(QCloseEvent *event);

    void showError(QString text);
    void saveIntervals();
    void startPlayer(IntervalTimestamp *stop = NULL, int selectCellRow = -1, int selectCellColumn = -1);
    void stopPlayer();
    void fillScriptProfiles();
    void fillScriptProfiles(QString scriptsDir);

private slots:
    void on_actionOpen_triggered();
    void on_playPausePushButton_clicked();
    void on_previousImagePushButton_clicked();
    void on_nextImagePushButton_clicked();
    void on_timeHorizontalSlider_sliderMoved(int position);
    void on_actionSave_triggered();
    void on_selectionChanged(const QItemSelection &, const QItemSelection &);
    void on_selectNextCell();
    void on_playIntervalPushButton_clicked();
    void on_insertPushButton_clicked();
    void on_deletePushButton_clicked();
    void on_reverseJumpPushButton_clicked();
    void on_forwardJumpPushButton_clicked();
    void on_actionAbout_triggered();
    void on_tableContextMenuRequested(QPoint position);
    void on_horizontalHeaderContextMenuRequested(QPoint position);
    void on_verticalHeaderContextMenuRequested(QPoint position);
    void on_editScript();    
    void on_action_Clear_triggered();
    void on_actionProfile_triggered(bool checked);
    void on_actionNew_triggered();
    void on_addNewScriptColumn_triggered();
    void on_addNewScriptRow_triggered();
    void on_removeScriptColumn_triggered();
    void on_removeScriptRow_triggered();
    void on_actionEdit_changed();
    void on_actionDelete_triggered();
    void on_intervalsTableView_doubleClicked(const QModelIndex &index);
    void showCurrentFrame(bool updateSlider = true);
    void videoPlayerStopped(int selectCellRow = -1, int selectCellColumn = -1);
};

#endif // MAINWINDOW_H
