#ifndef NAVIGATIONEVENTFILTER_H
#define NAVIGATIONEVENTFILTER_H

#include <QObject>

/**
 * @brief The NavigationEventFilter class
 * Keyboard press filter that filter cursor keys to suppress standard functions and allow custom key functions
 */
class NavigationEventFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event);

public:
    explicit NavigationEventFilter(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // NAVIGATIONEVENTFILTER_H
