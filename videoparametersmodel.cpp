#include "videoparametersmodel.h"

VideoParametersModel::VideoParametersModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

int VideoParametersModel::rowCount(const QModelIndex & /*parent*/) const
{
    return parameters.length();
}

int VideoParametersModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant VideoParametersModel::data(const QModelIndex &index, int role) const
{
    switch(role)
    {
    case Qt::DisplayRole:
        // time intervals
        if (index.row() < parameters.length()){
            switch(index.column()){
            case 0:
                return parameters[index.row()].first;
                break;
            case 1:
                return parameters[index.row()].second;
                break;
            }
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
        break;
    }
    return QVariant();
}

Qt::ItemFlags VideoParametersModel::flags(const QModelIndex &/*index*/) const
{
    return Qt::ItemIsEnabled;
}
