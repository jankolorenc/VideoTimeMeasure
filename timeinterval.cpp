#include "timeinterval.h"

TimeInterval::TimeInterval()
{
    start.isValid = stop.isValid = false;
}

bool TimeInterval::isDuration() const{
    return start.isValid && stop.isValid;
}

double TimeInterval::durationSeconds() const{
    if (start.isValid && stop.isValid && start.pts < stop.pts){
        return stop.pts - start.pts;
    }

    return 0;
}
