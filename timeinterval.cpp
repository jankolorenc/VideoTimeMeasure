#include "timeinterval.h"

TimeInterval::TimeInterval()
{
    start.isValid = stop.isValid = false;
}

bool TimeInterval::isDuration() const{
    return start.isValid && stop.isValid;
}

double TimeInterval::durationSeconds() const{
    if (start.isValid && stop.isValid && av_cmp_q(start.pts, stop.pts) == -1){
        return av_q2d(av_sub_q(stop.pts, start.pts));
    }

    return 0;
}
