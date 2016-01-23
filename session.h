#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <QDir>

/**
 * @brief The Session class
 * Session class store and restore last opened video
 */
class Session : public QObject
{
    Q_OBJECT

    //Q_PROPERTY(QString opennedVideo READ opennedVideo WRITE setOpennedVideo)
    //Q_PROPERTY(QString lastVideoDirectory READ lastVideoDirectory)

private:
    QString filename;
    QString videoFile;
    QString videoDirectory;

    void clear();

public:
    explicit Session(QObject *parent = 0);

    /**
     * @brief opennedVideo
     * @return openned vide file name
     */
    QString opennedVideo();

    /**
     * @brief lastVideoDirectory
     * @return video directory
     */
    QString lastVideoDirectory();

    /**
     * @brief save profile
     */
    void save();

    /**
     * @brief load profile
     */
    void load();

//public Q_SLOTS:
    /**
     * @brief setOpennedVideo
     * Set openned video file name to ssession
     * @param filename
     */
    void setOpennedVideo(const QString &filename);

signals:

public slots:

};

#endif // SESSION_H
