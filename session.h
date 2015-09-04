#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <QDir>

class Session : public QObject
{
    Q_OBJECT

    //Q_PROPERTY(QString opennedVideo READ opennedVideo WRITE setOpennedVideo)
    //Q_PROPERTY(QString lastVideoDirectory READ lastVideoDirectory)

private:
    QString filename = (QDir::homePath() + "/.VideoTimeMeasure/session.xml");
    QString videoFile;
    QString videoDirectory;

    void clear();

public:
    explicit Session(QObject *parent = 0);

    QString opennedVideo();
    QString lastVideoDirectory();

    void save();
    void load();

//public Q_SLOTS:
    void setOpennedVideo(const QString &filename);

signals:

public slots:

};

#endif // SESSION_H
