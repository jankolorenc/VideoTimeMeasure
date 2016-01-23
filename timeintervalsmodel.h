#ifndef TIMEINTERVALSMODEL_H
#define TIMEINTERVALSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QScriptEngine>
#include "timeinterval.h"
#include "tablescripts.h"

/**
 * @brief The TimeIntervalsModel class
 * Model to fill TableView with intervals (start, stop timestamps and duration) and profile script values
 */
class TimeIntervalsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    bool editingTableScripts;

    explicit TimeIntervalsModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & /*index*/) const;
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());
    bool insertColumns(int position, int columns, const QModelIndex &index=QModelIndex());
    bool removeColumns(int position, int columns, const QModelIndex &index=QModelIndex());
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    /**
     * @brief save intervals to file
     * @param fileName
     */
    void saveIntervals(QString fileName);

    /**
     * @brief load intervals from file
     * @param fileName
     */
    void loadIntervals(QString fileName);

    /**
     * @brief clear intervals
     */
    void clear();

    /**
     * @brief clear scripts
     */
    void clearTableScripts();

    /**
     * @brief get number of intervals
     * @return intervals count
     */
    int getIntervalsCount() const;

    /**
     * @brief get script from specified cell
     * @param row. -1 indicates whole column script
     * @param column. -1 indicates whole row script
     * @return script
     */
    QString getScript(int row, int column);

    /**
     * @brief set script to specified cell
     * @param row. -1 indicates whole column script
     * @param column. -1 indicates whole row script
     * @param script
     */
    void setScript(int row, int column, QString script);

    /**
     * @brief load scripts from directory (profile)
     * @param profile name
     * @param basePath profiles base path
     */
    void loadScriptProfile(QString profile, QString basePath);

    /**
     * @brief save sripts to directory
     * @param profile name
     */
    void saveScriptProfile(QString profile);

    /**
     * @brief delete script profile
     * @param profile name
     */
    void deleteScriptProfile(QString profile);

    /**
     * @brief get profiles directory
     * @return profiles directory
     */
    QString getProfilesDirectory();

    /**
     * @brief get current scripts profile
     * @return current scripts profile
     */
    QString getScriptsProfile();

signals:
    
public slots:
    //functions available in scripts
    /**
     * @brief return value of specified cell
     * @param row
     * @param column
     * @return
     */
    QScriptValue getValue(int row, int column) const;

    /**
     * @brief float value formatting in printf style
     * @param format
     * @param value
     * @return
     */
    QScriptValue printf(QString format, float value);

private:
    QList<TimeInterval> intervals;
    QScriptEngine engine;
    TableScripts tableScripts;
    /**
     * @brief convert table row to internal script rows.
     * Script row with 0 index is row after intervals total row
     * @param row in table
     * @return
     */
    int toScriptPositionRow(int row) const;
};

#endif // TIMEINTERVALSMODEL_H
