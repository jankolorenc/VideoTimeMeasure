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
    TableScripts();
    void Load(QDir directory);
};

#endif // TABLESCRIPTS_H
