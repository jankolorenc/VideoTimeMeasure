#ifndef INTERVALTIMESTAMP_H
#define INTERVALTIMESTAMP_H

#include <stdint.h>

typedef struct IntervalTimestamp {
    double pts;
    uint64_t dts;
    bool isValid;
} IntervalTimestamp;

#endif // INTERVALTIMESTAMP_H
