#ifndef VIDEOIMAGE_H
#define VIDEOIMAGE_H

#include <QImage>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/rational.h>
#ifdef __cplusplus
}
#endif

typedef struct VideoImage {
    QImage *image;
    AVRational pts;
} VideoImage;

#endif // VIDEIMAGE_H
