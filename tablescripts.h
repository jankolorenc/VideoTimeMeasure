#ifndef TABLESCRIPTS_H
#define TABLESCRIPTS_H

#include <QDir>

class TableScripts
{
private:
    QDir directory;
public:
    TableScripts();
    void Load(QDir directory);
};

#endif // TABLESCRIPTS_H
