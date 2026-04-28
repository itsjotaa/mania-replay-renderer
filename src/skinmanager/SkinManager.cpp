#include "skinmanager/SkinManager.hpp"
#include <miniz/miniz.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>

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

ManiaSkinConfig SkinManager::parseSkinIni(const std::string& content) {
    ManiaSkinConfig cfg;

    // find the [Mania] block with Keys: 4
    // there can be multiple [Mania] blocks for different keycounts
    std::istringstream stream(content);
    std::string line;

    bool inManiaBlock = false;
    bool foundKeys4   = false;

    while (std::getline(stream, line)) {
        // trim whitespace
        auto trim = [](std::string s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            size_t end   = s.find_last_not_of(" \t\r\n");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };
        line = trim(line);

        // skip comments
        if (line.empty() || line[0] == '/' || line[0] == '#') continue;

        // new section
        if (line == "[Mania]") {
            // if we already found the 4K block and parsed it, stop
            if (foundKeys4) break;
            inManiaBlock = true;
            continue;
        }

        // any other section header resets the block
        if (line[0] == '[') {
            if (foundKeys4) break;
            inManiaBlock = false;
            continue;
        }

        if (!inManiaBlock) continue;

        // parse key: value
        auto sep = line.find(':');
        if (sep == std::string::npos) continue;
        std::string key = trim(line.substr(0, sep));
        std::string val = trim(line.substr(sep + 1));

        if (key == "Keys") {
            if (val == "4") foundKeys4 = true;
            else { inManiaBlock = false; } // wrong keycount, skip block
        }

        if (!foundKeys4) continue;

        if (key == "ColumnStart") {
            cfg.columnStart = std::stoi(val);
        }
        else if (key == "HitPosition") {
            cfg.hitPosition = std::stoi(val);
        }
        else if (key == "ColumnWidth") {
            // parse comma-separated list: "45,45,45,45"
            std::istringstream vs(val);
            std::string token;
            int i = 0;
            while (std::getline(vs, token, ',') && i < 4) {
                cfg.columnWidths[i++] = std::stoi(trim(token));
            }
        }
        else if (key == "ColumnLineWidth") {
            std::istringstream vs(val);
            std::string token;
            int i = 0;
            while (std::getline(vs, token, ',') && i < 5) {
                cfg.columnLineWidths[i++] = std::stoi(trim(token));
            }
        }
    }

    return cfg;
}

void SkinManager::load(const std::string& oskPath) {
    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_file(&zip, oskPath.c_str(), 0)) {
        std::cerr << "[SkinManager] Failed to open .osk file: " << oskPath << "\n";
        std::exit(1);
    }

    // parse Skin.ini for layout config
    // try "Skin.ini" first (some skins use capital S), then "skin.ini"
    auto iniBuf = extractFileFromZip(zip, "Skin.ini");
    if (iniBuf.empty()) iniBuf = extractFileFromZip(zip, "skin.ini");

    if (!iniBuf.empty()) {
        std::string iniContent(iniBuf.begin(), iniBuf.end());
        config_ = parseSkinIni(iniContent);
        std::cout << "[SkinManager] ColumnStart=" << config_.columnStart
                  << " HitPosition=" << config_.hitPosition
                  << " ColumnWidths=" << config_.columnWidths[0]
                  << "," << config_.columnWidths[1]
                  << "," << config_.columnWidths[2]
                  << "," << config_.columnWidths[3] << "\n";
    } else {
        std::cout << "[SkinManager] Skin.ini not found, using defaults\n";
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

    // stage textures, global, not per column
    stageLeftTexture_   = textureFromBuffer(
        extractFileFromZip(zip, "mania-stage-left.png"),
        "mania-stage-left.png");

    stageRightTexture_  = textureFromBuffer(
        extractFileFromZip(zip, "mania-stage-right.png"),
        "mania-stage-right.png");

    stageBottomTexture_ = textureFromBuffer(
        extractFileFromZip(zip, "mania-stage-bottom.png"),
        "mania-stage-bottom.png");

    stageHintTexture_   = textureFromBuffer(
        extractFileFromZip(zip, "mania-stage-hint.png"),
        "mania-stage-hint.png");

    // hit burst textures — order matches Judgement enum
    hitTextures_[0] = textureFromBuffer(extractFileFromZip(zip, "mania-hit0.png"),   "mania-hit0.png");
    hitTextures_[1] = textureFromBuffer(extractFileFromZip(zip, "mania-hit50.png"),  "mania-hit50.png");
    hitTextures_[2] = textureFromBuffer(extractFileFromZip(zip, "mania-hit100.png"), "mania-hit100.png");
    hitTextures_[3] = textureFromBuffer(extractFileFromZip(zip, "mania-hit200.png"), "mania-hit200.png");
    hitTextures_[4] = textureFromBuffer(extractFileFromZip(zip, "mania-hit300.png"), "mania-hit300.png");
    hitTextures_[5] = textureFromBuffer(extractFileFromZip(zip, "mania-hit300g-0.png"), "mania-hit300g-0.png");

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
