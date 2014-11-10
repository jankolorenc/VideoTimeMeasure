#ifndef VIDEOIMAGE_H
#define VIDEOIMAGE_H

#include <QImage>
#include <stdint.h>

typedef struct VideoImage {
    QImage *image;
    double pts;
    uint64_t dts;
    // frame_duration = (1 + repeat_pict) * time_base
} VideoImage;

#endif // VIDEIMAGE_H
