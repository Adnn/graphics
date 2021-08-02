#pragma once

namespace ad {

struct Timer
{
    void mark(double aMonotonic)
    {
        mDelta = aMonotonic - mTime;
        mTime = aMonotonic;
    }

    double delta() const
    {
        return mDelta;
    }

    double time() const
    {
        return mTime;
    }

    double mTime;
    double mDelta;
};

} // namespace ad
