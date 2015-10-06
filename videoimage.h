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
    uint64_t dts;
    // frame_duration = (1 + repeat_pict) * time_base
} VideoImage;

#endif // VIDEIMAGE_H
