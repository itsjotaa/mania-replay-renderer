#include <iostream>
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include "renderer/Renderer.hpp"

int main() {
    auto replay  = parseOsr("test.osr");
    auto beatmap = parseOsu("test.osu");
    auto notes   = processReplay(beatmap, replay);

    ScrollCalculator scroll(beatmap.timingPoints, 10.0);
    Renderer renderer(1280, 720);

    // preview para verificar que se ve bien
    // renderer.preview(notes, scroll, replay, beatmap);

    // exportar a MP4
    renderer.exportVideo(notes, scroll, replay, beatmap, "output.mp4", "assets/audio.mp3");

    return 0;
}