#include "tablescripts.h"
#include "tablelimits.h"
#include <QRegExp>
#include <QPair>
#include <QFile>
#include <QTextStream>

/*
 * stores script for each table cell
 * expecting sparse matrix
 * requires fast read access
*/

TableScripts::TableScripts()
{
    lastRow = 0;
    lastColumn = FIXED_COLUMS - 1;
    setDefaultPath();
}

void TableScripts::setDefaultPath(){
    QString defaultPath = QDir::homePath().append("/.VideoTimeMeasure/scripts/default");
    directory.setPath(defaultPath);
}

bool TableScripts::checkDirectory(){
    if (!directory.exists()){
        if (!directory.mkpath(".")){
            qCritical(QString("Failed to create scripts directory ").append(directory.absolutePath()).toAscii());
            return FALSE;
        }
    }
    return TRUE;
}

void TableScripts::load(QString dir){
    directory.setPath(dir);
    if (!directory.exists()) return;

    // expecting filename format row-7_col-5.js
    lastRow = 0;
    lastColumn = FIXED_COLUMS - 1;
    QRegExp regex("(col|row)-(\\d+)");
    foreach (QString fileName, directory.entryList(QStringList("*.js"), QDir::Files|QDir::Readable, QDir::Unsorted)){
        int matchPos = 0;
        int row = -1, col = -1;
        while ((matchPos = regex.indexIn(fileName, matchPos)) != -1) {
            QString type = regex.cap(1);
            int index = regex.cap(2).toInt();
            if (type =="col"){
                if (lastColumn < index) lastColumn = index;
                col = index;
            }
            if (type =="row"){
                if (lastRow < index) lastRow = index;
                row = index;
            }
            matchPos += regex.matchedLength();
        }
        if (!(row == -1 && col == -1)){
            QFile file(directory.absoluteFilePath(fileName));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString script = in.readAll().trimmed();
                file.close();

                if (!script.isEmpty()){
                    if (row == -1) wholeColumnScripts[col] = script;
                    else{
                        if (col == -1) wholeRowScripts[row] = script;
                        else{
                            if (!cellScripts.contains(row)) cellScripts[row] = QMap<int, QString>();
                            cellScripts[row][col] = script;
                        }
                    }
                }
            }
        }
    }
}

void TableScripts::clear(){
    lastRow = 0;
    lastColumn = FIXED_COLUMS - 1;
    wholeColumnScripts.clear();
    wholeRowScripts.clear();
    cellScripts.clear();
    setDefaultPath();
}

QString TableScripts::getScript(int row, int column, bool exact = FALSE){
    if (row >= 0 && column >= 0 && cellScripts.contains(row) && cellScripts[row].contains(column))
        return cellScripts[row][column];

    if (((exact && row < 0) || !exact) && column >= 0 && wholeColumnScripts.contains(column))
        return wholeColumnScripts[column];

    if (((exact && column < 0) || !exact) && row >= 0 && wholeRowScripts.contains(row))
        return wholeRowScripts[row];

    return NULL;
}

void TableScripts::setScript(int row, int column, QString script){
    QString trimmed = script.trimmed();

    if (row < 0 && column >= 0){
        if (trimmed.isEmpty()) wholeColumnScripts.remove(column);
        else wholeColumnScripts[column] = trimmed;
        return;
    }

    if (column < 0 && row >= 0){
        if (trimmed.isEmpty()) wholeRowScripts.remove(column);
        else wholeRowScripts[row] = trimmed;
        return;
    }

    if (column >= 0 && row >= 0){
        if (trimmed.isEmpty()){
            if (cellScripts.contains(row)) cellScripts[row].remove(column);
        }
        else cellScripts[row][column] = trimmed;
    }
}

void TableScripts::saveScript(QString fileName, QString script){
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream out(&file);
        out << script;
        out.flush();
        file.close();
    }
    else qCritical(QString("Failed to write script ").append(fileName).toAscii());
}

void TableScripts::save(){
    if (!checkDirectory()) return;

    // remove existing scripts
    QRegExp regex("(col|row)-(\\d+)");
    foreach (QString fileName, directory.entryList(QStringList("*.js"), QDir::Files|QDir::Readable, QDir::Unsorted)){
        if (regex.indexIn(fileName) != -1) QFile::remove(directory.absoluteFilePath(fileName));
    }

    // generate new scripts
    foreach(int row, cellScripts.keys()){
        foreach(int col, cellScripts[row].keys()){
            QString script = cellScripts[row][col].trimmed();
            if (script.isEmpty()) continue;

            saveScript(directory.absoluteFilePath("row-%1_col-%2.js").arg(row).arg(col), script);
        }
    }

    foreach(int row, wholeRowScripts.keys()){
        QString script = wholeRowScripts[row].trimmed();
        if (script.isEmpty()) continue;

        saveScript(directory.absoluteFilePath("row-%1.js").arg(row), script);
    }

    foreach(int col, wholeColumnScripts.keys()){
        QString script = wholeColumnScripts[col].trimmed();
        if (script.isEmpty()) continue;

        saveScript(directory.absoluteFilePath("col-%1.js").arg(col), script);
    }
}

bool intLessThan(const int &v1, const int &v2)
{
    return v1 < v2;
}

bool intMoreThan(const int &v1, const int &v2)
{
    return v1 > v2;
}

template<typename T>
void TableScripts::insertItems(QMap<int, T > &map, int position, int count){
    QList<int> keys = map.keys();
    qSort(keys.begin(), keys.end(), intMoreThan);
    foreach(int key, keys){
        if (key >= position){
            map[key + count] = map[key];
            map.remove(key);
        }
    }
}

template<typename T>
void TableScripts::removeItems(QMap<int, T > &map, int position, int count){
    int maxRow = 0;
    // get max item index
    typename QMap<int, T >::iterator iterator;
    for (iterator = map.begin(); iterator != map.end(); ++iterator){
        if (iterator.key() > maxRow) maxRow = iterator.key();
    }

    // shift items
    for(int i = position; i <= maxRow; i++){
        if (map.contains(i + count)) map[i] = map[i + count];
        else map.remove(i);
    }
}

void TableScripts::insertRows(int position, int count){
    insertItems(cellScripts, position, count);
    insertItems(wholeRowScripts, position, count);
    lastRow += count;
}

void TableScripts::removeRows(int position, int count){
    removeItems(cellScripts, position, count);
    removeItems(wholeRowScripts, position, count);
    lastRow -=count;
}

void TableScripts::insertColumns(int position, int count){
    QMap<int, QMap<int, QString> >::iterator i;
    for (i = cellScripts.begin(); i != cellScripts.end(); ++i){
        insertItems(i.value(), position, count);
    }
    insertItems(wholeColumnScripts, position, count);
    lastColumn += count;
}

void TableScripts::removeColumns(int position, int count){
    QMap<int, QMap<int, QString> >::iterator i;
    for (i = cellScripts.begin(); i != cellScripts.end(); ++i){
        removeItems(i.value(), position, count);
    }
    removeItems(wholeColumnScripts, position, count);
    lastColumn -= count;
}
