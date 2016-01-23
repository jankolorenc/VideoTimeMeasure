#ifndef INTERVALTIMESTAMP_H
#define INTERVALTIMESTAMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/rational.h>
#ifdef __cplusplus
}
#endif

/**
  * Timestamp with validity flag
  * Mainly used as interval start or stop time
*/
typedef struct IntervalTimestamp {
    /**
     * @brief pts
     * Timestamp in FFMpeg pts
     */
    AVRational pts;

    /**
     * @brief isValid
     * Validity flag
     */
    bool isValid;
} IntervalTimestamp;

#endif // INTERVALTIMESTAMP_H
