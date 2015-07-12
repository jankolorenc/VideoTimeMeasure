#ifndef TIMEINTERVALSMODEL_H
#define TIMEINTERVALSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QScriptEngine>
#include "timeinterval.h"

class TimeIntervalsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TimeIntervalsModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & /*index*/) const;
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    void saveIntervals(QString fileName);
    void loadIntervals(QString fileName);
    void clear();
signals:
    
public slots:
    QScriptValue getValue(int column, int row);

private:
    QList<TimeInterval> intervals;
    QScriptEngine engine;
    //TableValue tableValue;
};

#endif // TIMEINTERVALSMODEL_H
