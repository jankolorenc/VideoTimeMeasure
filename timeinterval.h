#ifndef TIMEINTERVAL_H
#define TIMEINTERVAL_H

#include <stdint.h>
#include "intervaltimestamp.h"

class TimeInterval
{
public:
    TimeInterval();

    IntervalTimestamp start;
    IntervalTimestamp stop;

    bool isDuration() const;
    double durationSeconds() const;
};

#endif // TIMEINTERVAL_H
