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

void TableScripts::Load(QDir directory){
    this->directory = directory;
    // expecting filename format col-5_row-7.js
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
                    if (row == -1) wholeRowScripts[row] = script;
                    else{
                        if (col == -1) wholeColumnScripts[col] = script;
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

QString TableScripts::GetScript(int row, int column){
    if (cellScripts.contains(row) && cellScripts[row].contains(column)) return cellScripts[row][column];
    if (wholeColumnScripts.contains(column)) return wholeColumnScripts[column];
    if (wholeRowScripts.contains(row)) return wholeRowScripts[row];

    return NULL;
}


