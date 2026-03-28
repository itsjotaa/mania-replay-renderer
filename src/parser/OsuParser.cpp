#include "OsuParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

// helper: splits a string by a delimiter and returns the parts
// example: split("1,2,3", ',') -> {"1", "2", "3"}
static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, delim)) {
        parts.push_back(part);
    }
    return parts;
}

// helper: removes spaces and \r from the start and end of a string
// needed because .osu files sometimes use \r\n (Windows line endings)
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
        throw std::runtime_error("could not open: " + path);
    }

    BeatmapData data;
    data.keyCount = 0;

    // the file is divided into sections like [General], [HitObjects], etc.
    // this variable tracks which section we are currently in
    std::string currentSection = "";

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        // skip empty lines and comments
        if (line.empty() || line.substr(0, 2) == "//") continue;

        // detect section change
        // sections start with [ and end with ]
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line;
            continue;
        }

        // --- [Difficulty] section ---
        // read keycount (number of columns)
        if (currentSection == "[Difficulty]") {
            // each line has the format: Key: Value
            auto parts = split(line, ':');
            if (parts.size() < 2) continue;

            std::string key   = trim(parts[0]);
            std::string value = trim(parts[1]);

            // CircleSize in mania = number of columns
            // example: CircleSize:4 -> 4K
            if (key == "CircleSize") {
                data.keyCount = std::stoi(value);
            }
        }

        // --- [TimingPoints] section ---
        // each line defines a BPM point or scroll velocity
        if (currentSection == "[TimingPoints]") {
            // format: offset,msPerBeat,beats,sampleSet,sampleIndex,volume,isUninherited,effects
            // example: 1234,500,4,1,0,100,1,0
            //          1234,-100,4,1,0,100,0,0
            auto parts = split(line, ',');
            if (parts.size() < 8) continue;

            TimingPoint tp;
            tp.offset    = std::stoll(parts[0]);
            tp.msPerBeat = std::stod(parts[1]);

            // isUninherited field (parts[6]):
            // 1 = real BPM point (not inherited)
            // 0 = SV point (inherited from previous BPM)
            tp.isInherited = (std::stoi(parts[6]) == 0);

            data.timingPoints.push_back(tp);
        }

        // --- [HitObjects] section ---
        // each line is a note
        if (currentSection == "[HitObjects]") {
            // normal note format: x,y,time,type,hitSound,...
            // hold note format:   x,y,time,type,hitSound,endTime:...
            // normal example:     192,192,1234,1,0,0:0:0:0:
            // hold example:       192,192,1234,128,0,5678:0:0:0:0:
            auto parts = split(line, ',');
            if (parts.size() < 5) continue;

            int x          = std::stoi(parts[0]);
            long long time = std::stoll(parts[2]);
            int type       = std::stoi(parts[3]);

            Note note;
            note.startTime = time;

            // calculate column from x
            // osu! divides the width (512px) into keyCount equal sections
            // example for 4K:
            //   x=64  -> floor(64  * 4 / 512) = 0 -> column 0
            //   x=192 -> floor(192 * 4 / 512) = 1 -> column 1
            //   x=320 -> floor(320 * 4 / 512) = 2 -> column 2
            //   x=448 -> floor(448 * 4 / 512) = 3 -> column 3
            note.column = (int)std::floor((double)x * data.keyCount / 512.0);

            // determine if it is a hold note
            // bit 7 of the type field indicates hold: type & 128
            // example: type=128 -> hold only. type=129 -> hold + newCombo
            note.isHold = (type & 128) != 0;

            if (note.isHold) {
                // endTime is in parts[5], before the first ':'
                // example parts[5] = "5678:0:0:0:0:"
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
