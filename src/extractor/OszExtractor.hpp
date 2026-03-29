#pragma once
#include <string>
#include <vector>

struct OszContents {
    std::string osuPath;    // path to extracted .osu file
    std::string audioPath;  // path to extracted audio file
    bool success = false;
};

// extracts .osu and audio from a .osz file
// files are extracted to a temp directory
OszContents extractOsz(const std::string& oszPath, const std::string& beatmapMd5 = "");