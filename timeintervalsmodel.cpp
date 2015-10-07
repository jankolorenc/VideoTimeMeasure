#include "timeintervalsmodel.h"
#include "intervaltimestamp.h"
#include "tablelimits.h"
#include <QTime>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QColor>

Q_DECLARE_METATYPE(IntervalTimestamp)

TimeIntervalsModel::TimeIntervalsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    editingTableScripts = false;

    TimeInterval first;
    intervals.append(first);
}

int TimeIntervalsModel::intervalsCount() const{
    return intervals.length();
}

int TimeIntervalsModel::rowCount(const QModelIndex & /*parent*/) const
{
    return intervals.length() + tableScripts.rows + 1;
}

int TimeIntervalsModel::columnCount(const QModelIndex & /*parent*/) const
{
    return (tableScripts.columns < (FIXED_COLUMS - 1)) ? FIXED_COLUMS : tableScripts.columns;
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
                    return interval.addMSecs(av_q2d(intervals[index.row()].start.pts) * 1000).toString("hh:mm:ss.zzz");
                }
                return QVariant();
            case 1:
                if (intervals[index.row()].stop.isValid){
                    QTime interval(0,0,0);
                    return interval.addMSecs(av_q2d(intervals[index.row()].stop.pts) * 1000).toString("hh:mm:ss.zzz");
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

    case Qt::UserRole:
        if (index.row() < intervals.length()){
            switch(index.column()){
            case 0: return QVariant::fromValue(intervals[index.row()].start);
            case 1: return QVariant::fromValue(intervals[index.row()].stop);
            }
        }
        break;
    case Qt::BackgroundColorRole:
        QColor color;
        if (index.row() % 2) color.setNamedColor("whitesmoke");
        else color.setNamedColor("white");

        if (index.row() == intervalsCount()) color.setNamedColor("cyan");
        if (editingTableScripts && (index.row() > intervalsCount() || index.column() >= FIXED_COLUMS)){
            if (index.row() % 2) color.setNamedColor("powderblue");
            else color.setNamedColor("lightcyan");
        }

        return color;
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
    if (index.row() < intervals.length() && index.column() < FIXED_COLUMS - 1) return Qt::ItemIsSelectable |  Qt::ItemIsEnabled;
    else return Qt::ItemIsEnabled;
}

bool TimeIntervalsModel::insertRows(int position, int rows, const QModelIndex &index)
{
    bool result = true;

    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position + rows - 1);

    if (position <= intervals.length()){
        for (int row=0; row < rows; row++) {
            TimeInterval interval;
            intervals.insert(position, interval);
        }
    }
    else{
        if (rows > 0){
            tableScripts.insertRows(position - 1 - intervals.length(), rows);
        }
        else result = false;
    }

    endInsertRows();
    return result;
}

bool TimeIntervalsModel::removeRows(int position, int rows, const QModelIndex &index)
{
    bool result = false;
    Q_UNUSED(index);

    if (position == intervals.length()) return false;

    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    if (position < intervals.length()){
        for (int row=0; row < rows; ++row) {
            intervals.removeAt(position);
            result = true;
        }
    }
    else{
        if (position >= intervals.length() && rows > 0){
            tableScripts.removeRows(position - intervals.length() - 1, rows);
            result = true;
        }
        else result = false;
    }

    endRemoveRows();
    return result;
}

bool TimeIntervalsModel::insertColumns(int position, int columns, const QModelIndex &index)
{
    bool result = true;

    Q_UNUSED(index);
    beginInsertColumns(QModelIndex(), position, position + columns - 1);

    if (columns > 0){
        tableScripts.insertColumns(position, columns);
    }
    else result = false;

    endInsertColumns();

    return result;
}

bool TimeIntervalsModel::removeColumns(int position, int columns, const QModelIndex &index)
{
    bool result = false;
    Q_UNUSED(index);

    beginRemoveColumns(QModelIndex(), position, position + columns - 1);

    tableScripts.removeColumns(position, columns);

    endRemoveColumns();
    return result;
}

bool TimeIntervalsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole){
        if (index.row() < intervals.length()){
            // QModelIndex durationIndex = this->index(index.row(), 2);
            // QModelIndex totalDurationIndex = this->index(rowCount() - 1, 2);
            switch (index.column()){
            case 0:
                intervals[index.row()].start = value.value<IntervalTimestamp>();
                // emit(dataChanged(index, index));
                // emit(dataChanged(durationIndex, durationIndex));
                // emit(dataChanged(totalDurationIndex, totalDurationIndex));
                // update whole table because of scripts - too lazy to get dependency tree
                emit(layoutChanged());
                return true;
            case 1:
                intervals[index.row()].stop = value.value<IntervalTimestamp>();
                // emit(dataChanged(index, index));
                // emit(dataChanged(durationIndex, durationIndex));
                // emit(dataChanged(totalDurationIndex, totalDurationIndex));
                // update whole table because of scripts - too lazy to get dependency tree
                emit(layoutChanged());
                return true;
            }
        }
    }
    return false;
}

void xmlSaveTimestamp(IntervalTimestamp &timestamp, const QString &name, QXmlStreamWriter &stream){
    if (timestamp.isValid){
        stream.writeStartElement(name);
        //stream.writeAttribute("pts", QString("%1").arg(av_q2d(timestamp.pts)));
        //stream.writeAttribute("dts", QString("%1").arg(timestamp.dts));
        stream.writeAttribute("pts_num", QString("%1").arg(timestamp.pts.num));
        stream.writeAttribute("pts_den", QString("%1").arg(timestamp.pts.den));
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
        timestamp.pts = av_d2q(attributes.value("pts").toString().toDouble(), 100000);
        timestamp.isValid = true;
    }
    if (attributes.hasAttribute("pts_num") && attributes.hasAttribute("pts_den")){
        timestamp.pts = av_make_q(attributes.value("pts_num").toString().toInt(), attributes.value("pts_den").toString().toInt());
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

void TimeIntervalsModel::clearTableScripts(){
        beginResetModel();
        tableScripts.clear();
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
                    interval.start.pts = av_make_q(0, 1);
                    interval.start.isValid = false;
                    interval.stop.pts = av_make_q(0, 1);
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

QScriptValue TimeIntervalsModel::getValue(int row, int column) const
{
    if (row < intervals.length()){
        switch (column) {
        case 0:
            return intervals[row].start.isValid ? av_q2d(intervals[row].start.pts) : QScriptValue();
        case 1:
            return intervals[row].stop.isValid ? av_q2d(intervals[row].stop.pts) : QScriptValue();
        case 2:
            return intervals[row].isDuration() ? intervals[row].durationSeconds() : QScriptValue();
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

    QString script = tableScripts.getScript(toScriptPositionRow(row), column);

    if (script == NULL) return QScriptValue();

    engine.globalObject().setProperty("column", column);
    engine.globalObject().setProperty("row", row);
    engine.globalObject().setProperty("intervals", intervals.length());
    if (row < intervals.length()){
        engine.globalObject().setProperty("start", av_q2d(intervals[row].start.pts));
        engine.globalObject().setProperty("stop", av_q2d(intervals[row].stop.pts));
        engine.globalObject().setProperty("duration", intervals[row].durationSeconds());
    }
    else if (row == intervals.length()){
        double total = 0;
        for(int i = 0; i < intervals.length(); i++) if (intervals[i].isDuration()) total += intervals[i].durationSeconds();
        engine.globalObject().setProperty("duration", total);
    }
    QScriptValue objectValue = engine.newQObject(const_cast<TimeIntervalsModel *>(this));
    engine.globalObject().setProperty("table", objectValue);
    return engine.evaluate(script + "\n");
}

QScriptValue TimeIntervalsModel::printf(QString format, float value){
    QString str;
    str.sprintf(format.toAscii(),value);
    return str;
}

int TimeIntervalsModel::toScriptPositionRow(int row) const{
    return row - intervals.length() - 1;
}

QString TimeIntervalsModel::getScript(int row, int column){
    return tableScripts.getScript(toScriptPositionRow(row), column);
}

void TimeIntervalsModel::setScript(int row, int column, QString script){
    tableScripts.setScript(toScriptPositionRow(row), column, script);
}

void TimeIntervalsModel::loadScriptProfile(QString profile, QString basePath){
    beginResetModel();
    tableScripts.loadProfile(profile, basePath);
    endResetModel();
}

void TimeIntervalsModel::saveScriptProfile(QString profile){
    tableScripts.saveProfile(profile);
}

void TimeIntervalsModel::deleteScriptProfile(QString profile){
    beginResetModel();
    tableScripts.deleteProfile(profile, true);
    endResetModel();
}

QString TimeIntervalsModel::scriptsDirectory(){
    return tableScripts.basePath;
}

QString TimeIntervalsModel::scriptsProfile(){
    return tableScripts.profile;
}
