#ifndef TIMEINTERVALSMODEL_H
#define TIMEINTERVALSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QScriptEngine>
#include "timeinterval.h"
#include "tablescripts.h"

/**
 * @brief The TimeIntervalsModel class
 * Model to fill TableView with time intervals with intervals start, stop, duration and profile script values
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

    void saveIntervals(QString fileName);
    void loadIntervals(QString fileName);
    void clear();
    void clearTableScripts();
    int intervalsCount() const;
    QString getScript(int row, int column);
    void setScript(int row, int column, QString script);
    void loadScriptProfile(QString profile, QString basePath);
    void saveScriptProfile(QString profile);
    void deleteScriptProfile(QString profile);
    QString scriptsDirectory();
    QString scriptsProfile();

signals:
    
public slots:
    //available to scripts
    QScriptValue getValue(int row, int column) const;
    QScriptValue printf(QString format, float value);

private:
    QList<TimeInterval> intervals;
    QScriptEngine engine;
    TableScripts tableScripts;
    int toScriptPositionRow(int row) const;
};

#endif // TIMEINTERVALSMODEL_H
