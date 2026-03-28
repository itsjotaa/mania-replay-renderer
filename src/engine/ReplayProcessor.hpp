#pragma once 
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include <vector> 

// result of judging a note 
enum class Judgement {
    NONE,         // not yet judged (should not appear in the final result) 
    J320,         // perfect
    J300,         // excellent
    J200,         // good
    J100,         // ok
    J50,          // bad
    MISS,         // no input arrived in time 
};

// a beatmap note with its judgement result 
struct ProcessedNote { 
    Note note;              // original note (column, time, etc)
    Judgement judgement;    // how it was judged
    long long hitTime;      // when it was pressed(-1 if miss)
};

// main module function
std::vector<ProcessedNote> processReplay(
    const BeatmapData& beatmap,
    const ReplayData& replay
); 