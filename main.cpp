#include <QApplication>
#include <stdint.h>
#include "mainwindow.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif

int main(int argc, char *argv[])
{
    av_register_all();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
