#include "OszExtractor.hpp"
#include <miniz/miniz.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif
namespace fs = std::filesystem;

// write a buffer to a file
static bool writeFile(const fs::path& path, const void* data, size_t size) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data), size);
    return true;
}

// check if a filename is an audio file by extension
static bool isAudioFilename(const std::string& name) {
    auto lower = name;
    for (auto& c : lower) c = tolower(c);
    return lower.size() > 4 &&
           (lower.substr(lower.size() - 4) == ".mp3" ||
            lower.substr(lower.size() - 4) == ".ogg" ||
            lower.substr(lower.size() - 4) == ".wav");
}

// check if a filename is a .osu beatmap
static bool isOsuFilename(const std::string& name) {
    return name.size() > 4 &&
           name.substr(name.size() - 4) == ".osu";
}

OszContents extractOsz(const std::string& oszPath, const std::string& beatmapMd5) {
    OszContents result;

    // open the .osz as a zip archive
    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_file(&zip, oszPath.c_str(), 0)) {
        std::cerr << "Failed to open .osz file: " << oszPath << "\n";
        return result;
    }

    // create a temp directory for extracted files
    fs::path tmpDir = fs::temp_directory_path() / "mania-renderer-osz";
    fs::create_directories(tmpDir);

    int fileCount = mz_zip_reader_get_num_files(&zip);
    std::cout << "Found " << fileCount << " files in .osz\n";

    for (int i = 0; i < fileCount; i++) {
        // get file info
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat)) continue;

        std::string filename = stat.m_filename;

        // skip directories
        if (mz_zip_reader_is_file_a_directory(&zip, i)) continue;

        // only extract .osu and audio files
        bool isOsu   = isOsuFilename(filename);
        bool isAudio = isAudioFilename(filename);
        if (!isOsu && !isAudio) continue;

        // extract to temp dir
        fs::path outPath = tmpDir / filename;
        fs::create_directories(outPath.parent_path());
        
        // read file data
        size_t size = stat.m_uncomp_size;
        std::vector<char> data(size);
        if (!mz_zip_reader_extract_to_mem(&zip, i, data.data(), size, 0)) {
            std::cerr << "Failed to extract: " << filename << "\n";
            continue;
        }

        // write to disk
        if (!writeFile(outPath, data.data(), size)) {
            std::cerr << "Failed to write: " << outPath << "\n";
            continue;
        }

        std::cout << "Extracted: " << filename << "\n";

        if (isOsu) {
            std::cout << "Found .osu: " << filename << "\n";
            // check if this .osu matches the replay MD5
            if (!beatmapMd5.empty()) {
                std::string cmd = "md5sum \"" + outPath.string() + "\" 2>/dev/null";
                FILE* pipe = popen(cmd.c_str(), "r");
                char hash[64] = {};
                if (pipe) { fread(hash, 1, 32, pipe); pclose(pipe); }
                std::cout << "Calculated MD5: '" << std::string(hash, 32) << "'\n";
                std::cout << "Expected MD5:   '" << beatmapMd5 << "'\n";
                if (std::string(hash, 32) == beatmapMd5) {
                    result.osuPath = outPath.string();
                    std::cout << "Matched .osu: " << filename << "\n";
                }
            } else if (result.osuPath.empty()) {
                result.osuPath = outPath.string();
            }
        }
        if (isAudio && result.audioPath.empty()) {
            auto ext = outPath.extension().string();
            // prefer mp3/ogg over wav (wav files usually hitsounds)
            if (ext == ".mp3" || ext == ".ogg") {
                result.audioPath = outPath.string(); 
            }
        }
        // fallback: if no mp3/ogg found, use wav
        if (isAudio && result.audioPath.empty()) {
            result.audioPath = outPath.string();
        }
    }

    mz_zip_reader_end(&zip);

    result.success = !result.osuPath.empty();
    return result;
}