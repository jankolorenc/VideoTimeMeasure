#include "aspectratiopixmaplabel.h"
//#include <QDebug>

AspectRatioPixmapLabel::AspectRatioPixmapLabel(QWidget *parent) :
    QLabel(parent)
{
    this->setMinimumSize(1,1);
}

void AspectRatioPixmapLabel::setPixmap ( const QPixmap & p)
{
    pix = p;
    img = NULL;
    QLabel::setPixmap(p);
}

// supply full image for better scaling
void AspectRatioPixmapLabel::setImage ( QImage *i )
{
    img = i;
    if (i != NULL){
        pix = QPixmap::fromImage(img->scaled(this->width(), this->height(), Qt::KeepAspectRatio));
    }
    QLabel::setPixmap(pix);
}

int AspectRatioPixmapLabel::heightForWidth( int width ) const
{
    return ((qreal)pix.height()*width)/pix.width();
}

QSize AspectRatioPixmapLabel::sizeHint() const
{
    int w = this->width();
    return QSize( w, heightForWidth(w) );
}

void AspectRatioPixmapLabel::resizeEvent(QResizeEvent * e)
{
    (void)(e);
    if (img == NULL) QLabel::setPixmap(pix.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else setImage(img);
}
