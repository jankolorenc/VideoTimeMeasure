#ifndef TABLESCRIPTS_H
#define TABLESCRIPTS_H

#include <QDir>
#include <QMap>
#include "tablelimits.h"

#define DEFAULT_PROFILE "default"
#define DEFAULT_SCRIPTS_PATH "/.VideoTimeMeasure/scripts/"

class TableScripts
{

private:
    QMap<int, QString> wholeRowScripts;
    QMap<int, QString> wholeColumnScripts;
    QMap<int, QMap<int, QString> > cellScripts;

    void updateScriptPosition(QString type, int position, int &row, int &column);
    void saveScript(QString fileName, QString script);
    template<typename T>
    void insertItems(QMap<int, T> &map, int position, int count);
    template<typename T>
    int removeItems(QMap<int, T> &map, int position, int count);

public:
    int rows = 0;
    int columns = FIXED_COLUMS;
    QString basePath = (QDir::homePath() + DEFAULT_SCRIPTS_PATH);
    QString profile = DEFAULT_PROFILE;

    TableScripts();
    void loadProfile(QString profile = DEFAULT_PROFILE, QString basePath = (QDir::homePath() + DEFAULT_SCRIPTS_PATH));
    void saveProfile(QString profile = DEFAULT_PROFILE);
    void deleteProfile(QString profile, bool removeDirectory);
    void clear();
    QString getScript(int row, int column, bool exact = FALSE);
    void setScript(int row, int column, QString script);
    void insertRows(int position, int count);
    void removeRows(int position, int count);
    void insertColumns(int position, int count);
    void removeColumns(int position, int count);
};

#endif // TABLESCRIPTS_H
