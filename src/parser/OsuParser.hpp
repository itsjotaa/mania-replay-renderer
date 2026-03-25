#pragma once 
#include <string> 
#include <vector> 

//una nota del beatmap
struct Note {
    int column;         // 0-3 para 4K 
    long long startTime; // ms
    long long endTime;  // ms, igual que startime si no es hold note 
    bool isHold; 
};

// un punto de timing, define BPM o scroll velocity 
struct TimingPoint { 
    long long offset;   // en que momento aplica (ms)
    double msPerBeat;   // si es positivo: BPM real. si es negativo: SV
    bool isInherited;   // false = BPM, true = SV 
}; 

struct BeatmapData { 
    std::vector<Note> notes; 
    std::vector<TimingPoint> timingPoints; 
    int keyCount; 
}; 

BeatmapData parseOsu(const std::string& path); 