#pragma once 
#include <string> 
#include <vector> 

// beatmap note 
struct Note {
    int column;         // 0-3 for 4K 
    long long startTime; // ms
    long long endTime;  // ms, same as startime if not a hold note  
    bool isHold; 
};

// a timing point, defines BPM or scroll velocity 
struct TimingPoint { 
    long long offset;   // when this point takes effect (ms)
    double msPerBeat;   // if positive: real BPM. if negative: SV
    bool isInherited;   // false = BPM, true = SV 
}; 

struct BeatmapData { 
    std::vector<Note> notes; 
    std::vector<TimingPoint> timingPoints; 
    int keyCount; 
}; 

BeatmapData parseOsu(const std::string& path); 