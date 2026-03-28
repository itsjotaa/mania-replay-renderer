#pragma once 
#include <string>
#include <vector> 

// represent a point in time where the player 
// had certain keys pressed 
struct KeyFrame {
    long long timestamp; // absolute timestamp in ms from the start   
    int keys;            // bitmask: bit0=col1,  bit1=col2, bit2=col3, bit3=col4

};

// full replay information
struct ReplayData { 
    std::string beatmapHash;   // MD5 hash from the beatmap, used to identify which map was played   
    std::string playerName; 
    int count320;              // judgements counts (in mania: 320, 300, 200, 100, 50, miss)
    int count300;
    int count200;
    int count100;
    int count50;
    int countMiss; 
    int totalScore;
    int maxCombo; 
    std::vector<KeyFrame> frames; 
};

// main module function
ReplayData parseOsr(const std::string& path);