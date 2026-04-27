#include "skinmanager/SkinManager.hpp"
#include <miniz/miniz.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

// Searches an open zip archive for a file matching the given filename.
// The match is case-insensitive and ignores any subfolder prefix in the archive
// (e.g. "MySkin/mania-note1.png" matches when searching for "mania-note1.png").
// Returns the raw decompressed bytes, or an empty vector if not found.
std::vector<uint8_t> SkinManager::extractFileFromZip(mz_zip_archive& zip,
                                                       const std::string& filename) {
    int fileCount = (int)mz_zip_reader_get_num_files(&zip);

    for (int i = 0; i < fileCount; i++) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat)) continue;
        if (mz_zip_reader_is_file_a_directory(&zip, i)) continue;

        // get just the basename of the entry (strip subfolder prefix)
        std::string entryPath = stat.m_filename;
        std::string entryName = entryPath;
        auto slash = entryPath.rfind('/');
        if (slash != std::string::npos)
            entryName = entryPath.substr(slash + 1);

        // case-insensitive comparison
        auto toLower = [](std::string s) {
            for (auto& c : s) c = (char)tolower((unsigned char)c);
            return s;
        };
        if (toLower(entryName) != toLower(filename)) continue;

        // found — extract to heap
        size_t size = stat.m_uncomp_size;
        void* buf = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (!buf) {
            std::cerr << "[SkinManager] Failed to decompress: " << filename << "\n";
            return {};
        }

        // copy into a vector and free the miniz heap allocation
        std::vector<uint8_t> result(size);
        std::memcpy(result.data(), buf, size);
        mz_free(buf);
        return result;
    }

    return {}; // not found
}

// Loads a texture directly from a PNG buffer in memory.
// Exits if the buffer is empty (file wasn't in the archive) or if SFML
// can't decode it (corrupt or unsupported format).
sf::Texture SkinManager::textureFromBuffer(const std::vector<uint8_t>& buf,
                                            const std::string& assetName) {
    if (buf.empty()) {
        std::cerr << "[SkinManager] Missing required skin asset: "
                  << assetName << "\n";
        std::exit(1);
    }

    sf::Texture tex;
    if (!tex.loadFromMemory(buf.data(), buf.size())) {
        std::cerr << "[SkinManager] Failed to load skin asset: "
                  << assetName << "\n";
        std::exit(1);
    }

    // smooth scaling avoids pixelation when notes are stretched to column width
    tex.setSmooth(true);
    return tex;
}

void SkinManager::load(const std::string& oskPath) {
    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_file(&zip, oskPath.c_str(), 0)) {
        std::cerr << "[SkinManager] Failed to open .osk file: " << oskPath << "\n";
        std::exit(1);
    }

    // load only 2 unique column textures (1 and 2)
    for (int i = 0; i < 2; i++) {
        std::string c = std::to_string(i + 1); // "1" or "2"

        noteTextures_[i]   = textureFromBuffer(
            extractFileFromZip(zip, "mania-note" + c + ".png"),
            "mania-note" + c + ".png");

        lnHeadTextures_[i] = textureFromBuffer(
            extractFileFromZip(zip, "mania-note" + c + "H.png"),
            "mania-note" + c + "H.png");

        lnBodyTextures_[i] = textureFromBuffer(
            extractFileFromZip(zip, "mania-note" + c + "L.png"),
            "mania-note" + c + "L.png");

        keyUpTextures_[i]  = textureFromBuffer(
            extractFileFromZip(zip, "mania-key" + c + ".png"),
            "mania-key" + c + ".png");

        keyDownTextures_[i] = textureFromBuffer(
            extractFileFromZip(zip, "mania-key" + c + "D.png"),
            "mania-key" + c + "D.png");
    }

    mz_zip_reader_end(&zip);
    loaded_ = true;
    std::cout << "[SkinManager] Skin loaded from: " << oskPath << "\n";
}

const sf::Texture& SkinManager::getNoteTexture(int col) const {
    return noteTextures_[colToIdx(col)];
}
const sf::Texture& SkinManager::getLNHeadTexture(int col) const {
    return lnHeadTextures_[colToIdx(col)];
}
const sf::Texture& SkinManager::getLNBodyTexture(int col) const {
    return lnBodyTextures_[colToIdx(col)];
}
const sf::Texture& SkinManager::getLNTailTexture(int col) const {
    //no tail texture, reuse head (renderer will flip it)
    return lnHeadTextures_[colToIdx(col)]; 
}
const sf::Texture& SkinManager::getKeyTexture(int col, bool pressed) const {
    return pressed ? keyDownTextures_[colToIdx(col)] : keyUpTextures_[colToIdx(col)];
}
