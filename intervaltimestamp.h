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

typedef struct IntervalTimestamp {
    AVRational pts;
    bool isValid;
} IntervalTimestamp;

#endif // INTERVALTIMESTAMP_H
