#include <iostream>
#include <string>
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include "renderer/Renderer.hpp"
#include "ui/UI.hpp"

int main() {
    std::string osrPath, osuPath, audioPath;

    while (true) {
        int w = 1280, h = 720, fps = 60;
        double scroll = 10.0;

        UI ui(820, 600);
        bool doExport = ui.run(osrPath, osuPath, audioPath, w, h, fps, scroll);

        if (osrPath.empty() || osuPath.empty()) {
            break;
        }

        auto replay  = parseOsr(osrPath);
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

    return 0;
}