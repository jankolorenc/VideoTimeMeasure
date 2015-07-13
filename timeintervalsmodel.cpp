#include "timeintervalsmodel.h"
#include "intervaltimestamp.h"
#include <QTime>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

Q_DECLARE_METATYPE(IntervalTimestamp)

TimeIntervalsModel::TimeIntervalsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    TimeInterval first;
    intervals.append(first);
    //tableValue = new TableValue(NULL, engine);
}

int TimeIntervalsModel::intervalsCount(){
    return intervals.length();
}

int TimeIntervalsModel::rowCount(const QModelIndex & /*parent*/) const
{
    return ((tableScripts.lastRow < intervals.length()) ? intervals.length() : tableScripts.lastRow) + 1;
}

int TimeIntervalsModel::columnCount(const QModelIndex & /*parent*/) const
{
    return ((tableScripts.lastColumn < 2) ? 2 : tableScripts.lastColumn) + 1;
}

QVariant TimeIntervalsModel::data(const QModelIndex &index, int role) const
{
    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        // time intervals
        if (index.row() < intervals.length()){
            switch(index.column()){
            case 0:
                if (intervals[index.row()].start.isValid){
                    QTime interval(0,0,0);
                    return interval.addMSecs(intervals[index.row()].start.pts * 1000).toString("hh:mm:ss.zzz");
                }
                return QVariant();
            case 1:
                if (intervals[index.row()].stop.isValid){
                    QTime interval(0,0,0);
                    return interval.addMSecs(intervals[index.row()].stop.pts * 1000).toString("hh:mm:ss.zzz");
                }
                return QVariant();
            case 2:
                if (intervals[index.row()].isDuration()){
                    QTime interval(0,0,0);
                    return interval.addMSecs(intervals[index.row()].durationSeconds() * 1000).toString("hh:mm:ss.zzz");
                }
                return QVariant();
            }
        }
        if (index.row() == intervals.length()){
        // sum of intervals
            switch (index.column()) {
            case 0: return QVariant();
            case 1: return tr("Total");
            case 2:
                double total = 0;
                for(int i = 0; i < intervals.length(); i++) if (intervals[i].isDuration()) total += intervals[i].durationSeconds();
                QTime interval(0,0,0);
                return interval.addMSecs(total * 1000).toString("hh:mm:ss.zzz");
            }
        }
        return getValue(index.row(), index.column()).toString();

    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
        break;

    case Qt::UserRole:
        if (index.row() < intervals.length()){
            switch(index.column()){
            case 0: return QVariant::fromValue(intervals[index.row()].start);
            case 1: return QVariant::fromValue(intervals[index.row()].stop);
            }
        }
        break;
    }
    return QVariant();
}

QVariant TimeIntervalsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch (orientation)
        {
        case Qt::Horizontal:
            switch (section)
            {
            case 0:
                return QString(tr("Start"));
            case 1:
                return QString(tr("Stop"));
            case 2:
                return QString(tr("Duration"));
            }
            break;
        case Qt::Vertical:
            if (section < intervals.length()) return QString("%1").arg(section + 1);
            break;
        }
    }
    return QVariant();
}

Qt::ItemFlags TimeIntervalsModel::flags(const QModelIndex &index) const
{
    if (index.row() == rowCount(index) - 1) return Qt::ItemIsEnabled;
    if (index.column() == columnCount(index) - 1) return Qt::ItemIsEnabled;

    return Qt::ItemIsSelectable |  Qt::ItemIsEnabled ;
}

bool TimeIntervalsModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; row++) {
        TimeInterval interval;
        intervals.insert(position, interval);
    }

    endInsertRows();
    return true;
}

bool TimeIntervalsModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);

    if (position == intervals.length()) return false;

    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        intervals.removeAt(position);
    }

    endRemoveRows();
    return true;
}

bool TimeIntervalsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole){
        if (index.row() < intervals.length()){
            QModelIndex durationIndex = this->index(index.row(), 2);
            QModelIndex totalDurationIndex = this->index(rowCount() - 1, 2);
            switch (index.column()){
            case 0:
                intervals[index.row()].start = value.value<IntervalTimestamp>();
                emit(dataChanged(index, index));
                emit(dataChanged(durationIndex, durationIndex));
                emit(dataChanged(totalDurationIndex, totalDurationIndex));
                return true;
            case 1:
                intervals[index.row()].stop = value.value<IntervalTimestamp>();
                emit(dataChanged(index, index));
                emit(dataChanged(durationIndex, durationIndex));
                emit(dataChanged(totalDurationIndex, totalDurationIndex));
                return true;
            }
        }
    }
    return false;
}

void xmlSaveTimestamp(IntervalTimestamp &timestamp, const QString &name, QXmlStreamWriter &stream){
    if (timestamp.isValid){
        stream.writeStartElement(name);
        stream.writeAttribute("pts", QString("%1").arg(timestamp.pts));
        stream.writeAttribute("dts", QString("%1").arg(timestamp.dts));
        stream.writeEndElement(); // interval
    }
}

void TimeIntervalsModel::saveIntervals(QString fileName){
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QXmlStreamWriter stream(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        stream.writeStartElement("intervals");
        foreach (TimeInterval interval, intervals){
            stream.writeStartElement("interval");
            xmlSaveTimestamp(interval.start, "start", stream);
            xmlSaveTimestamp(interval.stop, "stop", stream);
            stream.writeEndElement(); // interval
        }
        stream.writeEndElement(); // intervals
        stream.writeEndDocument();
        file.close();
    }
}

void xmlLoadTimestamp(QXmlStreamReader &stream, IntervalTimestamp &timestamp){
    QXmlStreamAttributes attributes = stream.attributes();
    if (attributes.hasAttribute("pts") && attributes.hasAttribute("dts")){
        timestamp.pts = attributes.value("pts").toString().toDouble();
        timestamp.dts = attributes.value("dts").toString().toULong();
        timestamp.isValid = true;
    }
}

void TimeIntervalsModel::clear(){
        beginResetModel();
        intervals.clear();
        TimeInterval interval;
        intervals.append(interval);
        endResetModel();
}

void TimeIntervalsModel::loadIntervals(QString fileName){
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)){
        beginResetModel();
        intervals.clear();

        QXmlStreamReader stream(&file);
        TimeInterval interval;
        while(!stream.atEnd()){
            if (stream.readNextStartElement()){
                if (stream.name() == "interval"){
                    interval.start.pts = 0;
                    interval.start.dts = 0;
                    interval.start.isValid = false;
                    interval.stop.pts = 0;
                    interval.stop.dts = 0;
                    interval.stop.isValid = false;
                }
                if (stream.name() == "start") xmlLoadTimestamp(stream, interval.start);
                if (stream.name() == "stop") xmlLoadTimestamp(stream, interval.stop);
            }
            else{
                if (!stream.hasError()){
                    if (stream.name() == "interval"){
                        intervals.append(interval);
                    }
                }
            }
        }
        endResetModel();
    }
}

QScriptValue TimeIntervalsModel::getValue(int row, int column)
{
    if (row < intervals.length()){
        switch (column) {
        case 0:
            return intervals[row].start.isValid ? intervals[row].start.pts: QScriptValue::NullValue;
        case 1:
            return intervals[row].stop.isValid ? intervals[row].stop.pts : QScriptValue::NullValue;
        case 2:
            return intervals[row].isDuration() ? intervals[row].durationSeconds() : QScriptValue::NullValue;
        }
    }
    if (row == intervals.length()){
        switch (column) {
        case 1:
            return tr("Total");
        case 2:
            double total = 0;
            for(int i = 0; i < intervals.length(); i++) if (intervals[i].isDuration()) total += intervals[i].durationSeconds();
            return total;
        }
    }

    QString script = tableScripts.GetScript(row, column);

    if (script == NULL) return QScriptValue();

    engine.globalObject().setProperty("column", column);
    engine.globalObject().setProperty("row", row);
    engine.globalObject().setProperty("totalrow", intervals.length());
    if (row < intervals.length()){
        engine.globalObject().setProperty("start", intervals[row].start.pts);
        engine.globalObject().setProperty("stop", intervals[row].stop.pts);
        engine.globalObject().setProperty("difference", intervals[row].durationSeconds());
    }
    QScriptValue objectValue = engine.newQObject(this);
    engine.globalObject().setProperty("table", objectValue);
    return engine.evaluate(script.append("\n"));
}
