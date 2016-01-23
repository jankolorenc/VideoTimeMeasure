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

/**
  * Image with timestamp
 */
typedef struct VideoImage {
    QImage *image;

    /**
     * @brief pts
     * Timestamp in FFMpeg pts
     */
    AVRational pts;
} VideoImage;

#endif // VIDEIMAGE_H
