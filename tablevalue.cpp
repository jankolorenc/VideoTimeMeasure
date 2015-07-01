#include "tablevalue.h"
#include <QVariant>

TableValue::TableValue(QObject *parent) :
    QObject(parent)
{
}

QVariant TableValue::getCurrentValue()
{
    return QVariant();
}

QVariant TableValue::getValueRelative(int column, int row)
{
    return QVariant();

}

QVariant TableValue::getValueAbsolute(int column, int row)
{
    return QVariant();;
}
