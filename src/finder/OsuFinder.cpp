#include "OsuFinder.hpp"
#include "util/md5.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>

namespace fs = std::filesystem;

// read entire file into a byte vector
static std::vector<uint8_t> readFile(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}

// calculate MD5 of a file on disk
static std::string fileMd5(const fs::path& path) {
    std::string cmd = "md5sum \"" + path.string() + "\" 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char result[64] = {};
    fread(result, 1, 32, pipe);
    pclose(pipe);
    return std::string(result, 32);
}

// check if a file looks like a .osu beatmap
// (starts with "osu file format")
static bool isOsuFile(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char header[16] = {};
    f.read(header, 15);
    return std::string(header).find("osu file format") != std::string::npos;
}

// check if a file is an audio file by its first bytes (magic bytes)
static bool isAudioFile(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    uint8_t header[4] = {};
    f.read(reinterpret_cast<char*>(header), 4);

    // MP3: starts with ID3 or 0xFF 0xFB
    if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') return true;
    if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) return true;

    // OGG: starts with OggS
    if (header[0] == 'O' && header[1] == 'g' && header[2] == 'g' && header[3] == 'S') return true;

    return false;
}

// get the lazer files directory
static fs::path getLazerFilesDir() {
    const char* home = getenv("HOME");
    if (!home) return {};
    return fs::path(home) / ".local/share/osu/files";
}

BeatmapFiles findBeatmap(const std::string& beatmapMd5) {
    BeatmapFiles result;

    fs::path filesDir = getLazerFilesDir();
    if (!fs::exists(filesDir)) {
        std::cerr << "osu! lazer files directory not found\n";
        return result;
    }

    std::cout << "Searching for beatmap with MD5: " << beatmapMd5 << "\n";

    // search all files in the lazer file store
    // lazer stores files as: files/XX/XXYY.../fullhash
    // we check each file to see if it's a .osu and if its MD5 matches

    for (const auto& entry : fs::recursive_directory_iterator(filesDir)) {
        if (!entry.is_regular_file()) continue;

        // only check files without extension (lazer format)
        // or with .osu extension (stable format)
        auto ext = entry.path().extension().string();
        if (!ext.empty() && ext != ".osu") continue;

        // quick check: is it a .osu file?
        if (!isOsuFile(entry.path())) continue;

        // calculate MD5 and compare
        std::string md5 = fileMd5(entry.path());
        if (md5 != beatmapMd5) continue;

        // found the .osu file
        result.osuPath = entry.path().string();
        std::cout << "Found beatmap: " << result.osuPath << "\n";

        // now find the audio file in the same directory
        // in lazer, audio files are in the same folder level
        // we search the parent directories for an audio file
        fs::path searchDir = entry.path().parent_path().parent_path();
        std::cout << "Searching audio in: " << searchDir << "\n";
        for (const auto& audioEntry : fs::recursive_directory_iterator(searchDir)) {
            if (!audioEntry.is_regular_file()) continue;
            std::cout << "Checking" << audioEntry.path() << "\n"; 
            auto audioExt = audioEntry.path().extension().string();
            if (!audioExt.empty() && audioExt != ".mp3" && audioExt != ".ogg") continue;
            if (isAudioFile(audioEntry.path())) {
                result.audioPath = audioEntry.path().string();
                std::cout << "Found audio: " << result.audioPath << "\n";
                break;
            }
        }

        result.found = true;
        return result;
    }

    std::cerr << "Beatmap not found\n";
    return result;
}