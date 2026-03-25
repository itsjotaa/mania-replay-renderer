#pragma once 
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include <vector> 

//el resultado de juzgar una nota 
enum class Judgement {
    NONE,         // todavia no se juzgo (no deberia aparecer en el resultado final) 
    J320,         // perfecto
    J300,         // excelente 
    J200,         // bueno 
    J100,         // ok
    J50,          // mal
    MISS,         // no llego ningun input a tiempo 
};

// una nots del beatmap con su resultado 
struct ProcessedNote { 
    Note note;              // la nota original (colunma, tiempo, etc)
    Judgement judgement;    // como salio 
    long long hitTime;      // en que momento se presiono (-1 si miss)
};

// funcion principal del modulo 
std::vector<ProcessedNote> processReplay(
    const BeatmapData& beatmap,
    const ReplayData& replay
); 