#ifndef ASPECTRATIOPIXMAPLABEL_H
#define ASPECTRATIOPIXMAPLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

/**
 * @brief The AspectRatioPixmapLabel class
 *
 * Image label That keeps aspect ration when scaling
 */
class AspectRatioPixmapLabel : public QLabel
{
    Q_OBJECT
public:
    explicit AspectRatioPixmapLabel(QWidget *parent = 0);

    /**
     * @brief compute height fo desired width
     * @param width
     * @return height for specified width
     */
    virtual int heightForWidth( int width ) const;

    virtual QSize sizeHint() const;

    /**
     * @brief set image
     */
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
