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
#include "session.h"

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
    bool videoLoaded;

    Session session;

    Ui::MainWindow *ui;

    VideoPlayer videoPlayer;

    TimeIntervalsModel *timeIntervals;

    QActionGroup *scriptProfilesActionGroup;

    void closeEvent(QCloseEvent *event);

    /**
     * @brief show error in message box
     * @param text error
     */
    void showError(QString text);

    void saveIntervals();

    void startPlayer(IntervalTimestamp *stop = NULL, int selectCellRow = -1, int selectCellColumn = -1);

    void stopPlayer();

    /**
     * @brief fill drop down menu with script profiles
     */
    void fillScriptProfiles();

    /**
     * @brief fill drop down menu with script profiles
     * @param scriptsDir profiles directory
     */
    void fillScriptProfiles(QString scriptsDir);

    /**
     * @brief register new script profile menu item and connect signals
     * @param name of profile
     * @return
     */
    QAction *registerScriptProfile(QString name);

    void showCurrentPlayerImage(bool updateSlider = true);

    void openFile(QString fileName);

private slots:

    /**
     * @brief open video file and intervals
     */
    void on_actionOpen_triggered();

    void on_playPausePushButton_clicked();

    void on_previousImagePushButton_clicked();

    void on_nextImagePushButton_clicked();

    /**
     * @brief timescale slide moved
     * @param position
     */
    void on_timeHorizontalSlider_sliderMoved(int position);

    /**
     * @brief save video file intervals
     */
    void on_actionSave_triggered();

    /**
     * @brief selected timestamp in intervals table changed
     */
    void on_selectionChanged(const QItemSelection &, const QItemSelection &);

    void on_selectNextCell();

    void on_playIntervalPushButton_clicked();

    /**
     * @brief insert new interval
     */
    void on_insertPushButton_clicked();

    /**
     * @brief delete interval
     */
    void on_deletePushButton_clicked();

    /**
     * @brief jump in reverse direction
     */
    void on_reverseJumpPushButton_clicked();

    /**
     * @brief jump forward button
     */
    void on_forwardJumpPushButton_clicked();
    void on_actionAbout_triggered();

    /**
     * @brief show context menu in intervals table
     * @param position
     */
    void on_tableContextMenuRequested(QPoint position);

    /**
     * @brief show context menu in intervals table horizontal header
     * @param position
     */
    void on_horizontalHeaderContextMenuRequested(QPoint position);

    /**
     * @brief show context menu in intervals table vertical header
     * @param position
     */
    void on_verticalHeaderContextMenuRequested(QPoint position);

    /**
     * @brief show edit script request
     */
    void on_editScript();

    /**
     * @brief clear script request
     */
    void on_action_Clear_triggered();

    /**
     * @brief load selected scripts profile in menu
     * @param checked
     */
    void on_actionProfile_triggered(bool checked);

    /**
     * @brief create new scripts profile
     */
    void on_actionNew_triggered();

    void on_addNewScriptColumn_triggered();

    void on_addNewScriptRow_triggered();

    void on_removeScriptColumn_triggered();

    void on_removeScriptRow_triggered();

    /**
     * @brief toggle scripts editing mode
     */
    void on_actionEdit_changed();

    /**
     * @brief delete scripts profile
     */
    void on_actionDelete_triggered();

    /**
     * @brief show edit script for specified cell when double clicked
     * @param index
     */
    void on_intervalsTableView_doubleClicked(const QModelIndex &index);

    void on_showCurrentFrame();

    /**
     * @brief make actions when player stopped
     * @param selectCellRow
     * @param selectCellColumn
     */
    void videoPlayerStopped(int selectCellRow = -1, int selectCellColumn = -1);

    /**
     * @brief move to next timestamp cell in intervals table
     */
    void on_nextCellPushButton_clicked();

    /**
     * @brief generate scripts profile examples
     */
    void on_action_Get_examples_triggered();

    void on_actionRead_me_triggered();

    /**
     * @brief export current script profile
     */
    void on_actionExport_triggered();

    /**
     * @brief import scripts profile
     */
    void on_actionImport_triggered();

protected:
     void dragEnterEvent(QDragEnterEvent *event);
     void dropEvent(QDropEvent *event);
};

#endif // MAINWINDOW_H
