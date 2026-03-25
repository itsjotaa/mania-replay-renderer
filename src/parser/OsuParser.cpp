#include "OsuParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

// helper: divide un string por un delimitador y devuelve las partes
// ejemplo: split("1,2,3", ',') → {"1", "2", "3"}
static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, delim)) {
        parts.push_back(part);
    }
    return parts;
}

// helper: elimina espacios y \r al inicio y final de un string
// necesario porque los archivos .osu a veces tienen \r\n (Windows)
static std::string trim(const std::string& s) {
    int start = 0;
    int end = s.size() - 1;
    while (start <= end && (s[start] == ' ' || s[start] == '\r')) start++;
    while (end >= start && (s[end] == ' '  || s[end] == '\r'))   end--;
    return s.substr(start, end - start + 1);
}

BeatmapData parseOsu(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("no se pudo abrir: " + path);
    }

    BeatmapData data;
    data.keyCount = 0;

    // el archivo está dividido en secciones como [General], [HitObjects], etc.
    // esta variable guarda en qué sección estamos parados
    std::string currentSection = "";

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        // ignorar líneas vacías y comentarios
        if (line.empty() || line.substr(0, 2) == "//") continue;

        // detectar cambio de sección
        // las secciones empiezan con [ y terminan con ]
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line;
            continue;
        }

        // --- sección [General] ---
        // acá leemos el keycount (columnas del mapa)
        if (currentSection == "[Difficulty]") {
            // cada línea tiene el formato: Clave: Valor
            auto parts = split(line, ':');
            if (parts.size() < 2) continue;

            std::string key   = trim(parts[0]);
            std::string value = trim(parts[1]);

            // CircleSize en mania = cantidad de columnas
            // ejemplo: CircleSize:4 → 4K
            if (key == "CircleSize") {
                data.keyCount = std::stoi(value);
            }
        }

        // --- sección [TimingPoints] ---
        // cada línea define un punto de BPM o de scroll velocity
        if (currentSection == "[TimingPoints]") {
            // formato: offset,msPerBeat,beats,sampleSet,sampleIndex,volume,isUninherited,effects
            // ejemplo: 1234,500,4,1,0,100,1,0
            //          1234,-100,4,1,0,100,0,0
            auto parts = split(line, ',');
            if (parts.size() < 8) continue;

            TimingPoint tp;
            tp.offset    = std::stoll(parts[0]);
            tp.msPerBeat = std::stod(parts[1]);

            // el campo isUninherited (parts[6]):
            // 1 = punto de BPM real (NO heredado)
            // 0 = punto de SV (heredado del BPM anterior)
            tp.isInherited = (std::stoi(parts[6]) == 0);

            data.timingPoints.push_back(tp);
        }

        // --- sección [HitObjects] ---
        // cada línea es una nota
        if (currentSection == "[HitObjects]") {
            // formato normal:  x,y,time,type,hitSound,...
            // formato hold:    x,y,time,type,hitSound,endTime:...
            // ejemplo nota:    192,192,1234,1,0,0:0:0:0:
            // ejemplo hold:    192,192,1234,128,0,5678:0:0:0:0:
            auto parts = split(line, ',');
            if (parts.size() < 5) continue;

            int x         = std::stoi(parts[0]);
            long long time = std::stoll(parts[2]);
            int type      = std::stoi(parts[3]);

            Note note;
            note.startTime = time;

            // calcular columna a partir de x
            // osu! divide el ancho (512px) en keyCount secciones iguales
            // ejemplo para 4K:
            //   x=64  → floor(64  * 4 / 512) = 0 → columna 0
            //   x=192 → floor(192 * 4 / 512) = 1 → columna 1
            //   x=320 → floor(320 * 4 / 512) = 2 → columna 2
            //   x=448 → floor(448 * 4 / 512) = 3 → columna 3
            note.column = (int)std::floor((double)x * data.keyCount / 512.0);

            // determinar si es hold note
            // el bit 7 del campo type indica hold: type & 128
            // ejemplo: type=128 → solo hold. type=129 → hold + newCombo
            note.isHold = (type & 128) != 0;

            if (note.isHold) {
                // el endTime está en parts[5], antes del primer ':'
                // ejemplo parts[5] = "5678:0:0:0:0:"
                auto endParts = split(parts[5], ':');
                note.endTime = std::stoll(endParts[0]);
            } else {
                note.endTime = note.startTime;
            }

            data.notes.push_back(note);
        }
    }

    return data;
}