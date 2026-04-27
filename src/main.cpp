#include <iostream>
#include <string>
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include "renderer/Renderer.hpp"
#include "skinmanager/SkinManager.hpp"
#include "ui/UI.hpp"
#include "finder/OsuFinder.hpp"
#include "extractor/OszExtractor.hpp"

int main() {
    std::string osrPath, osuPath, oskPath;

    while (true) {
        int w = 1280, h = 720, fps = 60;
        double scroll = 10.0;

        UI ui(820, 600);
        bool doExport = ui.run(osrPath, osuPath, oskPath, w, h, fps, scroll);
        if (osrPath.empty()) {
            break;
        }

        // parse replay first to get the beatmap MD5
        auto replay = parseOsr(osrPath);

        // .osz handling: extract audio, then let OsuFinder locate the correct .osu
        std::string audioPath;
        if (osuPath.size() > 4 && osuPath.substr(osuPath.size() - 4) == ".osz") {
            std::cout << "Extracting audio from .osz...\n";
            auto extracted = extractOsz(osuPath, "");
            if (!extracted.audioPath.empty())
                audioPath = extracted.audioPath;
            osuPath = ""; // clear so OsuFinder picks the correct .osu below
        }

        // auto-detect the correct .osu via OsuFinder
        if (osuPath.empty()) {
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

        std::cout << "Notes in beatmap: " << beatmap.notes.size() << "\n";
        std::cout << "Frames in replay: " << replay.frames.size() << "\n";
        std::cout << "Key count: " << beatmap.keyCount << "\n";

        // load skin from the selected .osk — exits if any asset is missing
        SkinManager skin;
        skin.load(oskPath);

        Renderer renderer(w, h);
        renderer.setSkin(skin);

        if (doExport) {
            renderer.exportVideo(notes, scrollCalc, replay, beatmap,
                                 "output.mp4", audioPath, fps);
            break;
        } else {
            renderer.preview(notes, scrollCalc, replay, beatmap);
        }
    }

    return 0;
}