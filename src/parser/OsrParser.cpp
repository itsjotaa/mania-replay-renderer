#include "OsrParser.hpp"
#include <cstdio> 
#include <stdexcept>
#include <sstream> 
#include <lzma.h>

// helper: read a string with osu! special format 
static std::string readOsuString(FILE* f) {
    uint8_t indicator; 
    fread(&indicator, 1, 1, f);
    
    if (indicator == 0x00) {
        return ""; // empty string  
    }

    // read size in ULEB128
    // short strings (< 128 chars) are 1 byte
    // read more bytes for longer strings   
    uint32_t length = 0; 
    int shift = 0; 
    while (true) {
        uint8_t byte; 
        fread(&byte, 1, 1, f);
        length |= (byte & 0x7F) << shift; // take 7 useful bits  
        if ((byte & 0x80) == 0) break;    // if bit 8 is 0, ends
        shift += 7; 
    }

    std::string result(length, '\0'); 
    fread(&result[0], 1, length, f);
    return result; 
}

static std::vector<KeyFrame> parseFrames(const std::vector<uint8_t>& compressed) { 
    // buffer for compressed data
    std::vector<uint8_t> decompressed(10 *1024 * 1024); 

    lzma_stream strm = LZMA_STREAM_INIT; 
    //LZMA_ALONE is the format osu! uses 
    lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX); 
    if (ret != LZMA_OK) {
        throw std::runtime_error("failed to initialize LZMA decoder"); 
    }

    strm.next_in = compressed.data(); 
    strm.avail_in = compressed.size(); 
    strm.next_out = decompressed.data();
    strm.avail_out = decompressed.size(); 

    lzma_code(&strm, LZMA_FINISH); 
    size_t totalOut = decompressed.size() - strm.avail_out; 
    lzma_end(&strm); 

    // convert descompressed bytes to text string
    std::string text(decompressed.begin(), decompressed.begin() + totalOut); 

    // parse frame per frame
    // format for each frame is: delta|keys|x|y 
    // frames are separated by commas  
    std::vector<KeyFrame> frames; 
    long long currentTime = 0; 

    std::stringstream ss(text); 
    std::string frameStr; 

    while (std::getline(ss, frameStr, ',')) {
        if (frameStr.empty()) continue; 

        //split 4 fields by '|'
        std::stringstream ss2(frameStr); 
        std::string part; 
        std::vector<std::string> parts; 
        while (std::getline(ss2, part, '|')) {
            parts.push_back(part); 
        }
        if (parts.size() < 2) continue;

        long long delta = std::stoll(parts[0]); 
        int keys        = std::stoi(parts[1]); 

        // negatives deltas are metadata frames. skip them
        if (delta < 0) continue; 

        currentTime += delta; 
        frames.push_back({currentTime, keys});
    }

    return frames; 
}

ReplayData parseOsr(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb"); 
    if (!f) throw std::runtime_error("could not open: " + path); 

    ReplayData data; 

    // game mode (1 byte), should be 3 for mania 
    uint8_t mode; 
    fread(&mode, 1, 1, f); 
    if (mode != 3) throw std::runtime_error("replay is not osu!mania"); 

    // client version (4 bytes), need to read it to advance file position  

    uint32_t version; 
    fread(&version, 4, 1, f); 

    // header string 
    data.beatmapHash = readOsuString(f); 
    data.playerName  = readOsuString(f);
    readOsuString(f); // hash replay  
    
    //judgements (2 bytes each one)
    uint16_t v; 
    fread(&v, 2, 1, f); data.count300  = v; 
    fread(&v, 2, 1, f); data.count100  = v; 
    fread(&v, 2, 1, f); data.count50   = v; 
    fread(&v, 2, 1, f); data.count320  = v; 
    fread(&v, 2, 1, f); data.count200  = v; 
    fread(&v, 2, 1, f); data.countMiss = v; 

    // score and combo (4 and 2 bytes)
    uint32_t score;
    fread(&score, 4, 1, f);
    data.totalScore = score;

    fread(&v, 2, 1, f);
    data.maxCombo = v;

    // perfect flag (1 byte), not needed 
    uint8_t perfect;
    fread(&perfect, 1, 1, f);

    // mods (4 bytes), not needed for now 
    uint32_t mods;
    fread(&mods, 4, 1, f);

    // health bard and timestamp, not needed
    readOsuString(f);
    uint64_t timestamp;
    fread(&timestamp, 8, 1, f);

    // read compressed block
    uint32_t compressedSize;
    fread(&compressedSize, 4, 1, f);

    std::vector<uint8_t> compressed(compressedSize);
    fread(compressed.data(), 1, compressedSize, f);

    fclose(f);

    // decompress and parse frames
    data.frames = parseFrames(compressed);

    return data;
}