#pragma once
#include <string>

struct BeatmapFiles {
    std::string osuPath;    // path to the .osu file
    std::string audioPath;  // path to the audio file
    bool found = false;
};

// finds the beatmap files corresponding to a replay
// by searching the osu! lazer file store
BeatmapFiles findBeatmap(const std::string& beatmapMd5);