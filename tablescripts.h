#ifndef TABLESCRIPTS_H
#define TABLESCRIPTS_H

#include <QDir>
#include <QMap>

class TableScripts
{

private:
    QDir directory;
    QMap<int, QString> wholeRowScripts;
    QMap<int, QString> wholeColumnScripts;
    QMap<int, QMap<int, QString> > cellScripts;

public:
    int lastRow;
    int lastColumn;

    TableScripts();
    void Load(QDir directory);
    QString GetScript(int row, int column);
};

#endif // TABLESCRIPTS_H
