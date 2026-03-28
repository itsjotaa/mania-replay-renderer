#include "ScrollCalculator.hpp"
#include <cmath>

ScrollCalculator::ScrollCalculator(
    const std::vector<TimingPoint>& timingPoints,
    double scrollSpeed
) : timingPoints_(timingPoints), scrollSpeed_(scrollSpeed) {}

// calculates the visual distance between two moments considering SVs
// result is in "time units adjusted by SV"
double ScrollCalculator::getDistance(long long from, long long to) const {
    if (from == to) return 0.0;

    // determine the active SV at each moment
    // iterate timing points and accumulate per segment
    double total = 0.0;
    double currentSV = 1.0;  // default SV if no timing point exists
    long long segmentStart = from;

    for (const auto& tp : timingPoints_) {
        // only care about SVs (inherited timing points)
        // BPM points do not directly affect scroll speed
        if (!tp.isInherited) continue;

        // if this SV starts after 'to', it does not affect us
        if (tp.offset >= to) break;

        // if this SV starts before 'from', update current SV
        // but do not accumulate distance yet
        if (tp.offset <= from) {
            // sv = -100 / msPerBeat
            // example: msPerBeat=-50  -> sv=2.0 (double speed)
            //          msPerBeat=-200 -> sv=0.5 (half speed)
            currentSV = -100.0 / tp.msPerBeat;
            continue;
        }

        // this SV starts within the range [from, to]
        // accumulate the previous segment with the active SV
        total += (tp.offset - segmentStart) * currentSV;
        segmentStart = tp.offset;
        currentSV = -100.0 / tp.msPerBeat;
    }

    // accumulate the last segment up to 'to'
    total += (to - segmentStart) * currentSV;

    return total;
}

float ScrollCalculator::getNoteY(
    long long noteTime,
    long long currentTime,
    int hitY
) const {
    // distance between current time and when the note arrives
    // if noteTime > currentTime -> note is in the future -> above hitY
    // if noteTime < currentTime -> note already passed -> below hitY
    double distance = getDistance(currentTime, noteTime);

    // convert distance to pixels
    // scrollSpeed_ controls how many pixels per ms-unit
    // 0.45 is an empirical scale factor for a good visual result
    float pixels = (float)(distance * scrollSpeed_ * 0.45);

    // hitY is where notes are judged (bottom area)
    // notes coming in the future are higher up (lower Y value)
    return hitY - pixels;
}

bool ScrollCalculator::isVisible(
    long long noteTime,
    long long currentTime,
    int hitY,
    int height
) const {
    float y = getNoteY(noteTime, currentTime, hitY);
    // visible if within screen bounds with a small margin
    // the margin prevents notes from popping in and out abruptly
    return y >= -50 && y <= height + 50;
}
