#include <iostream>
#include <string>
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include "renderer/Renderer.hpp"
#include "ui/UI.hpp"
#include "finder/OsuFinder.hpp"
#include "extractor/OszExtractor.hpp"

int main() {
    std::string osrPath, osuPath, audioPath;

    while (true) {
        int w = 1280, h = 720, fps = 60;
        double scroll = 10.0;

        UI ui(820, 600);
        bool doExport = ui.run(osrPath, osuPath, audioPath, w, h, fps, scroll);
        std::cout << "osrPath: " << osrPath << "\n";
        std::cout << "osuPath: " << osuPath << "\n";
        std::cout << "audioPath: " << audioPath << "\n";

        if (osrPath.empty()) {
            break;
        }

        // parse replay first to get the beatmap MD5
         auto replay  = parseOsr(osrPath);

        // if user selected a .osz, extract the audio from it
        if (osuPath.size() > 4 && osuPath.substr(osuPath.size() - 4) == ".osz") {
            std::cout << "Extracting audio from .osz...\n";
            auto extracted = extractOsz(osuPath, ""); // empty MD5 = just get audio
            if (extracted.success || !extracted.audioPath.empty()) {
                audioPath = extracted.audioPath;
                std::cout << "Audio extracted: " << audioPath << "\n";
            }
            osuPath = ""; // clear so OsuFinder can find the correct .osu
        }

        // auto-detect the correct .osu using OsuFinder
        if (osuPath.empty()) {
            std::cout << "Auto-detecting beatmap...\n";
            auto found = findBeatmap(replay.beatmapHash);
            if (found.found) {
                osuPath = found.osuPath;
                if (audioPath.empty()) audioPath = found.audioPath;
            }
            if (osuPath.empty()) {
                std::cerr << "Could not find beatmap. Please select it manually.\n";
                continue;
            }
        }
        auto beatmap = parseOsu(osuPath);
        auto notes   = processReplay(beatmap, replay);
        ScrollCalculator scrollCalc(beatmap.timingPoints, scroll);
        Renderer renderer(w, h);

        if (doExport) {
            renderer.exportVideo(notes, scrollCalc, replay, beatmap, "output.mp4", audioPath, fps);
            break;
        } else {
            renderer.preview(notes, scrollCalc, replay, beatmap);
        }
    }

    std::cout << "Using osu: " << osuPath << "\n";
    std::cout << "Using audio: " << audioPath << "\n";

    return 0;
}