#include "ReplayProcessor.hpp"
#include <cmath>
#include <algorithm>

// las ventanas de tiempo para cada jugador (en ms)
// si la diferencia entre el hit y la nota es menor a este valor -> ese judgement
static const long long WINDOW_320 = 16; 
static const long long WINDOW_300 = 40; 
static const long long WINDOW_200 = 73; 
static const long long WINDOW_100 = 103; 
static const long long WINDOW_50 = 127; 
static const long long WINDOW_MISS = 161; 

// convierte una diferencia de tiempo a un judgement 
// diff = |tiempo_hit - tiempo_nota| en ms 
static Judgement getJudgement(long long diff) { 
    if (diff <= WINDOW_320) return Judgement::J320; 
    if (diff <= WINDOW_300) return Judgement::J300;
    if (diff <= WINDOW_200) return Judgement::J200; 
    if (diff <= WINDOW_100) return Judgement::J100; 
    if (diff <= WINDOW_50) return Judgement::J50; 
    return Judgement::MISS; 
}

// verifica si la columna 'col' esta activa en el bitmask 'keys'
// col va de 0  3 
// ejemplo: col=2, keys=0110 -> (0110 >> 2) & 1 = 1 -> activa 
static bool isColumnActive(int keys, int col) {
    return ((keys >> col) & 1) != 0; 
}

std::vector<ProcessedNote> processReplay(
    const BeatmapData& beatmap, 
    const ReplayData& replay 
) {
    // inicializar todas las notas como MISS sin hit 
    std::vector<ProcessedNote> result; 
    for (const auto& note : beatmap.notes) {
        result.push_back({note, Judgement::MISS, -1}); 
    }

    // para cada columna, llevar registro de que notas ya fueron asignadas
    // esto evita que un keypress matchee dos notas a la vez 
    // ejemplo: si notes[0] y notes[1] son de col=0 y notes[0] ya fue hit, 
    // el proximo keypress en col=0 solo puede matchear notes[1] en adelante
    std::vector<int> nextUnmatched(beatmap.keyCount, 0); 

    //recorrer todos los frames del replay buscando keypresses nuevos
    int prevKeys = 0; // estado de teclado en el frame anterior

    long long firstNoteTime = result.empty() ? 0 : result[0].note.startTime;
    printf("firstNoteTime = %lld\n", firstNoteTime); 

    for (const auto& frame : replay.frames) {
        int currKeys = frame.keys; 

        // ignorar frames que ocurren antes de que empiece el mapa 
        // (son inputs del menu de osu!)
        if (frame.timestamp < firstNoteTime - WINDOW_MISS) {
            prevKeys = currKeys; // actualizar igual para no acumular diferencias
            continue; 
        }

        // detectar que columnas fueron presionadas en este frame 
        // usando XOR para encontrar bits que cambiaron 
        // y AND con currKeys para quedarnos solo con los que pasaron de 0->1
        int newlyPressed = (currKeys ^ prevKeys) & currKeys; 

        // para cada columna posible (0 a keyCount-1)
        for (int col = 0; col < beatmap.keyCount; col++) {

            // verificar si esta columna especifica fue presionada ahora
            if (!isColumnActive(newlyPressed, col)) continue; 

            // buscar la nota mas cercana sin asignar en esta columna 
            // partimos desde nextUnmatched[col] para no revisar notas ya asignadas 
            int bestIdx = -1; 
            long long bestDiff = WINDOW_MISS + 1; // peor que miss 

            for (int i = nextUnmatched[col]; i < (int)result.size(); i++) {
                // solo considerar notas de esta columna 
                if (result[i].note.column != col) continue; 

                // si esta nota ya fye asignada, saltarla
                if (result[i].hitTime != -1) continue; 

                long long diff = std::abs(frame.timestamp - result[i].note.startTime);

                // si la nota ya paso por mas de WINDOW_MISS, no tiene sentido 
                // seguir buscando, las siguientes notas son aun mas lejanas 
                if (frame.timestamp - result[i].note.startTime > WINDOW_MISS) { 
                    //actualizar nextUnmatched para no revisar esa nota de nuevo 
                    nextUnmatched[col] = i + 1; 
                    continue;
                }

                // si la nota esta muy en el futuro, tampoco sirve 
                if (result[i].note.startTime - frame.timestamp > WINDOW_MISS) break; 

                // esta nota esta dentro de la ventana -> es candidat 
                if (diff < bestDiff) { 
                    bestDiff = diff; 
                    bestIdx = i; 
                }
            }

            // si encontramos una nota candidata, asignarla
            if (bestIdx != -1) {
                result[bestIdx].judgement = getJudgement(bestDiff); 
                result[bestIdx].hitTime   = frame.timestamp; 
            }
            // si no encontramos ninguna -> keypress sin nota (no afecta nada)
        }

        prevKeys = currKeys; // actualizar el estado anterior
    }

    // las noras que quedaron con hitTime == -1 ya estan marcadas como MISS 
    return result; 
}