#include "ScrollCalculator.hpp"
#include <cmath>

ScrollCalculator::ScrollCalculator(
    const std::vector<TimingPoint>& timingPoints,
    double scrollSpeed
) : timingPoints_(timingPoints), scrollSpeed_(scrollSpeed) {}

// calcula la distancia visual entre dos momentos considerando SVs
// el resultado es en "unidades de tiempo ajustadas por SV"
double ScrollCalculator::getDistance(long long from, long long to) const {
    if (from == to) return 0.0;

    // determinar el SV vigente en cada momento
    // recorremos los timing points y acumulamos por segmento
    double total = 0.0;
    double currentSV = 1.0;  // SV por defecto si no hay ningún timing point
    long long segmentStart = from;

    for (const auto& tp : timingPoints_) {
        // solo nos interesan los SVs (timing points heredados)
        // los BPM points no afectan el scroll speed directamente
        if (!tp.isInherited) continue;

        // si este SV empieza después de 'to', ya no nos afecta
        if (tp.offset >= to) break;

        // si este SV empieza antes de 'from', actualizar el SV actual
        // pero no acumular distancia todavía
        if (tp.offset <= from) {
            // sv = -100 / msPerBeat
            // ejemplo: msPerBeat=-50 → sv=2.0 (doble velocidad)
            //          msPerBeat=-200 → sv=0.5 (mitad de velocidad)
            currentSV = -100.0 / tp.msPerBeat;
            continue;
        }

        // este SV empieza dentro del rango [from, to]
        // acumular el segmento anterior con el SV vigente
        total += (tp.offset - segmentStart) * currentSV;
        segmentStart = tp.offset;
        currentSV = -100.0 / tp.msPerBeat;
    }

    // acumular el último segmento hasta 'to'
    total += (to - segmentStart) * currentSV;

    return total;
}

float ScrollCalculator::getNoteY(
    long long noteTime,
    long long currentTime,
    int hitY
) const {
    // distancia entre el momento actual y cuando llega la nota
    // si noteTime > currentTime → la nota está en el futuro → arriba de hitY
    // si noteTime < currentTime → la nota ya pasó → abajo de hitY
    double distance = getDistance(currentTime, noteTime);

    // convertir distancia a píxeles
    // scrollSpeed_ controla cuántos píxeles por ms-unidad
    // el valor 0.45 es un factor de escala empírico para que se vea bien
    float pixels = (float)(distance * scrollSpeed_ * 0.45);

    // hitY es donde se juzgan las notas (parte inferior)
    // las notas que vienen en el futuro están más arriba (Y menor)
    return hitY - pixels;
}

bool ScrollCalculator::isVisible(
    long long noteTime,
    long long currentTime,
    int hitY,
    int height
) const {
    float y = getNoteY(noteTime, currentTime, hitY);
    // visible si está dentro de la pantalla con un margen extra
    // el margen evita que las notas aparezcan y desaparezcan bruscamente
    return y >= -50 && y <= height + 50;
}