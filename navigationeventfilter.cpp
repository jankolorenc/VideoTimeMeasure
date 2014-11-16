#include "navigationeventfilter.h"
#include<QKeyEvent>

NavigationEventFilter::NavigationEventFilter(QObject *parent) :
    QObject(parent)
{
}

bool NavigationEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up
                || keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right
                || keyEvent->key() == Qt::Key_PageDown || keyEvent->key() == Qt::Key_PageUp
                || keyEvent->key() == Qt::Key_Home || keyEvent->key() == Qt::Key_End) {
            // do nothing

            //qDebug("Ate key press %d", keyEvent->key());
            return true;
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}
