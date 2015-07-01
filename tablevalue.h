#ifndef TABLEVALUE_H
#define TABLEVALUE_H

#include <QObject>

class TableValue : public QObject
{
    Q_OBJECT
public:
    explicit TableValue(QObject *parent = 0);

    QVariant getCurrentValue();
    QVariant getValueRelative(int column, int row);
    QVariant getValueAbsolute(int column, int row);

signals:

public slots:

};

#endif // TABLEVALUE_H
