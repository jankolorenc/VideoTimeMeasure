#ifndef ASPECTRATIOPIXMAPLABEL_H
#define ASPECTRATIOPIXMAPLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

class AspectRatioPixmapLabel : public QLabel
{
    Q_OBJECT
public:
    explicit AspectRatioPixmapLabel(QWidget *parent = 0);
    virtual int heightForWidth( int width ) const;
    virtual QSize sizeHint() const;
    void setImage (QImage *);
signals:

public slots:
    void setPixmap ( const QPixmap & );
    void resizeEvent(QResizeEvent *);
private:
    QPixmap pix;
    QImage *img;
};

#endif // ASPECTRATIOPIXMAPLABEL_H
