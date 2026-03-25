#pragma once 
#include <string>
#include <vector> 

// representa un momento en el tuempo donde el jugador 
// tenia ciertas teclas presiondas 
struct KeyFrame {
    long long timestamp; // tiempo absoluto en ms desde el inicio 
    int keys;            // bitmask: bit0=col1,  bit1=col2, bit2=col3, bit3=col4

};

// toda la informacion del replay 
struct ReplayData { 
    std::string beatmapHash;   // MD5 del beatmap, para identificar cual es 
    std::string playerName; 
    int count320;              // judgements (en mania: 320, 300, 200, 100, 50, miss)
    int count300;
    int count200;
    int count100;
    int count50;
    int countMiss; 
    int totalScore;
    int maxCombo; 
    std::vector<KeyFrame> frames; 
};

// funcion principal del modulo 
ReplayData parseOsr(const std::string& path);