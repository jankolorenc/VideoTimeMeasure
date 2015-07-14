#include "tablescripts.h"
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
    lastColumn = 0;
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

void TableScripts::load(){
    if (!checkDirectory()) return;

    // expecting filename format row-7_col-5.js
    lastRow = 0;
    lastColumn = 0;
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
