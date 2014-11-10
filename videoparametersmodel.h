#ifndef VIDEOPARAMETERSMODEL_H
#define VIDEOPARAMETERSMODEL_H

#include <QAbstractTableModel>

class VideoParametersModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit VideoParametersModel(QObject *parent = 0);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex & /*index*/) const;

    QList<QPair<QString, QString> > parameters;

signals:
    
public slots:
    
};

#endif // VIDEOPARAMETERSMODEL_H
