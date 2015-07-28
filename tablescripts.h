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

    void updateScriptPosition(QString type, int position, int &row, int &column);
    bool checkDirectory();
    void saveScript(QString fileName, QString script);
    void setDefaultPath();

public:
    int lastRow;
    int lastColumn;

    TableScripts();
    void load(QString dir);
    void save();
    void clear();
    QString getScript(int row, int column, bool exact = FALSE);
    void setScript(int row, int column, QString script);
};

#endif // TABLESCRIPTS_H
