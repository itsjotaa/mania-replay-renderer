#pragma once
#include "parser/OsuParser.hpp"

class ScrollCalculator {
public:
    // constructor: takes timing points and base scroll speed
    // scrollSpeed is a global multiplier (e.g. 20.0 is a typical value)
    ScrollCalculator(const std::vector<TimingPoint>& timingPoints, double scrollSpeed);

    // returns the Y position in pixels of a note
    // noteTime:    when the note arrives (ms)
    // currentTime: current video time (ms)
    // hitY:        Y coordinate of the judgement line in pixels (e.g. 650)
    // height:      total screen height (e.g. 720)
    float getNoteY(long long noteTime, long long currentTime, int hitY) const;

    // returns true if the note is visible on screen at currentTime
    bool isVisible(long long noteTime, long long currentTime, int hitY, int height) const;

private:
    // calculates accumulated distance between two moments considering SVs
    // positive = noteTime is in the future relative to currentTime
    double getDistance(long long from, long long to) const;

    std::vector<TimingPoint> timingPoints_;
    double scrollSpeed_;
};
