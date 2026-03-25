#pragma once
#include "parser/OsuParser.hpp"

class ScrollCalculator {
public:
    // constructor: recibe los timing points y el scroll speed base
    // scrollSpeed es un multiplicador global (ej: 20.0 es un valor típico)
    ScrollCalculator(const std::vector<TimingPoint>& timingPoints, double scrollSpeed);

    // devuelve la posición Y en píxeles de una nota
    // noteTime:    cuando llega la nota (ms)
    // currentTime: el momento actual del video (ms)
    // hitY:        la coordenada Y de la línea de juicio en píxeles (ej: 650)
    // height:      altura total de la pantalla (ej: 720)
    float getNoteY(long long noteTime, long long currentTime, int hitY) const;

    // devuelve true si la nota es visible en pantalla en currentTime
    bool isVisible(long long noteTime, long long currentTime, int hitY, int height) const;

private:
    // calcula la distancia acumulada entre dos momentos considerando SVs
    // positivo = noteTime está en el futuro respecto a currentTime
    double getDistance(long long from, long long to) const;

    std::vector<TimingPoint> timingPoints_;
    double scrollSpeed_;
};