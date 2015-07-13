#include "tablescripts.h"
#include <QRegExp>
#include <QPair>

/*
 * stores script for each table cell
 * expecting sparse matrix
 * requires fast read access
*/

TableScripts::TableScripts()
{

}

void TableScripts::Load(QDir directory){
    this->directory = directory;
    // expecting filename format col-5_row-7.js
    int rows = 0, columns = 0;
    QRegExp regex("(col|row)-(\\d+)");
    foreach (QString file, directory.entryList(QStringList("*.js"), QDir::Files|QDir::Readable, QDir::Unsorted)){
        int matchPos = 0;
        int row = -1, col = -1;
        while ((matchPos = regex.indexIn(file, matchPos)) != -1) {
            QString type = regex.cap(1);
            int index = regex.cap(2).toInt();
            if (type =="col"){
                if (columns < index) columns = index;
                col = index;
            }
            if (type =="row"){
                if (rows < index) rows = index;
                row = index;
            }
            matchPos += regex.matchedLength();
        }
        if (!(row == -1 && col == -1)){
                if (row == -1) wholeRowScripts[row] = file;
                else{
                    if (col == -1) wholeColumnScripts[col] = file;
                    else{
                        if (!cellScripts.contains(row)) cellScripts[row] = QMap<int, QString>();
                        cellScripts[row][col] = file;
                    }
                }
        }
    }
}
