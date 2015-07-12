#include "tablescripts.h"
#include "cellscript.h"
#include <QRegExp>
#include <QPair>

TableScripts::TableScripts()
{

}

void TableScripts::Load(QDir directory){
    this->directory = directory;
    // expecting filename format col-5_row-7.js
    QList<CellScript *> scriptsList;
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
        if (row != -1 || col != -1){
            CellScript *cellScript = new CellScript();
            cellScript->column = col;
            cellScript->row = row;
            scriptsList.append(cellScript);
        }
    }
}
