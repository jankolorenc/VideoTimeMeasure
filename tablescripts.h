#ifndef TABLESCRIPTS_H
#define TABLESCRIPTS_H

#include <QDir>
#include <QMap>
#include "tablelimits.h"

#define DEFAULT_PROFILE "default"
#define DEFAULT_SCRIPTS_PATH "/.VideoTimeMeasure/scripts/"

/**
 * @brief The TableScripts class
 * Class containg cripts for table cells, rows and columns
 */
class TableScripts
{

private:
    QMap<int, QString> wholeRowScripts;
    QMap<int, QString> wholeColumnScripts;
    QMap<int, QMap<int, QString> > cellScripts;

    /**
     * @brief save script to specified file
     * @param fileName
     * @param script
     */
    void saveScript(QString fileName, QString script);

    /**
     * @brief insert items to given QMap at specities positon (map key). Shift following items.
     * @param map
     * @param position
     * @param count
     */
    template<typename T>
    void insertItems(QMap<int, T> &map, int position, int count);

    /**
     * @brief remove items from given QMap at specified position (pak key) and shring map.
     * @param map
     * @param position
     * @param count
     * @return number of removed items
     */
    template<typename T>
    int removeItems(QMap<int, T> &map, int position, int count);

public:
    int rows;
    int columns;

    /**
     * @brief script profiles path
     */
    QString basePath;

    /**
     * @brief current profile name
     */
    QString profile;

    TableScripts();

    void loadProfile(QString profile = DEFAULT_PROFILE, QString basePath = (QDir::homePath() + DEFAULT_SCRIPTS_PATH));

    void saveProfile(QString profile = DEFAULT_PROFILE);

    /**
     * @brief delete scripts from profile
     * @param profile
     * @param removeDirectory if true, profile directory is also removed
     */
    void deleteProfile(QString profile, bool removeDirectory);

    /**
     * @brief clear all scripts and set default profile name
     */
    void clear();

    /**
     * @brief get script from specified cell
     * @param row
     * @param column
     * @param exact if false, whole column script then whole row script is returned when cell contain no script
     * @return
     */
    QString getScript(int row, int column, bool exact = false) const;

    /**
     * @brief set script to specified cell.
     * @param row if negative, whole column script is set to specified column
     * @param column if negative, whole row script is set to specified row
     * @param script
     */
    void setScript(int row, int column, QString script);

    /**
     * @brief insert rows to script table
     * @param position
     * @param count
     */
    void insertRows(int position, int count);

    /**
     * @brief remove rows from script table
     * @param position
     * @param count
     */
    void removeRows(int position, int count);

    /**
     * @brief insert columns to script table
     * @param position
     * @param count
     */
    void insertColumns(int position, int count);

    /**
     * @brief remove columns from script table
     * @param position
     * @param count
     */
    void removeColumns(int position, int count);
};

#endif // TABLESCRIPTS_H
