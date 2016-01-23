#ifndef TIMEINTERVAL_H
#define TIMEINTERVAL_H

#include <stdint.h>
#include "intervaltimestamp.h"

/**
 * @brief The TimeInterval class
 * Time interval with start and stop timestamps
 */
class TimeInterval
{
public:
    TimeInterval();

    /**
     * @brief start timestamp
     */
    IntervalTimestamp start;

    /**
     * @brief stop timestamp
     */
    IntervalTimestamp stop;

    /**
     * @brief isDuration
     * @return indication whether duration defined by start and stop timestaps is valid
     */
    bool isDuration() const;

    /**
     * @brief durationSeconds
     * @return dutaion in seconds
     */
    double durationSeconds() const;
};

#endif // TIMEINTERVAL_H
