#include "session.h"
#include <QXmlStreamWriter>

Session::Session(QObject *parent) :
    QObject(parent)
{
    filename = (QDir::homePath() + "/.VideoTimeMeasure/session.xml");
}

QString Session::opennedVideo(){
    return videoFile;
}

QString Session::lastVideoDirectory(){
    return videoDirectory;
}

void Session::setOpennedVideo(const QString &filename){
    videoFile = filename;
    if (!filename.isEmpty()){
        videoDirectory = QFileInfo(filename).absoluteDir().absolutePath();
        save();
    }
}

void Session::clear(){
    videoFile.clear();
    videoDirectory.clear();
}

void Session::save(){
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QXmlStreamWriter stream(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        stream.writeStartElement("session");

        stream.writeStartElement("video");

        stream.writeStartElement("file");
        stream.writeCharacters(videoFile);
        stream.writeEndElement(); // file

        stream.writeStartElement("directory");
        stream.writeCharacters(videoDirectory);
        stream.writeEndElement(); // directory

        stream.writeEndElement(); // video

        stream.writeEndElement(); // session
        stream.writeEndDocument();
        file.close();
    }
}

void Session::load(){
    bool videoSection = false;
    QFile file(filename);
    if (file.open(QFile::ReadOnly | QFile::Text)){
        QXmlStreamReader stream(&file);
        clear();
        while(!stream.atEnd()){
            if (stream.readNextStartElement()){
                if (stream.name() == "video") videoSection = true;
                if(videoSection){
                    if (stream.name() == "file"){
                        videoFile = stream.readElementText();
                    }
                    if (stream.name() == "directory"){
                        videoDirectory = stream.readElementText();
                    }
                }
            }
        }
    }
}
