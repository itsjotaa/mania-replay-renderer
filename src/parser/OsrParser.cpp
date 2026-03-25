#include "OsrParser.hpp"
#include <cstdio> 
#include <stdexcept>
#include <sstream> 
#include <lzma.h>

// helper: lee un string con el formato especial de osu! 
// formato: [1 byte indicador] [1 byte tamano] [N bytes de texto]
static std::string readOsuString(FILE* f) {
    uint8_t indicator; 
    fread(&indicator, 1, 1, f);
    
    if (indicator == 0x00) {
        return ""; // string vacio 
    }

    // leer tamano en ULEB128
    // para string cortos (< 128 chars) es solo 1 byte
    // para strings mas largos se leen mas bytes 
    uint32_t length = 0; 
    int shift = 0; 
    while (true) {
        uint8_t byte; 
        fread(&byte, 1, 1, f);
        length |= (byte & 0x7F) << shift; //tomar los 7 bits utiles 
        if ((byte & 0x80) == 0) break;    // si el bit 8 es 0, terminamos
        shift += 7; 
    }

    std::string result(length, '\0'); 
    fread(&result[0], 1, length, f);
    return result; 
}

static std::vector<KeyFrame> parseFrames(const std::vector<uint8_t>& compressed) { 
    //buffer para los datos descomprimidos
    std::vector<uint8_t> decompressed(10 *1024 * 1024); 

    lzma_stream strm = LZMA_STREAM_INIT; 
    //LZMA_ALONE es el formato que usa osu! (disntinto al formato .xz)
    lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX); 
    if (ret != LZMA_OK) {
        throw std::runtime_error("error iniciando el decoder LZMA"); 
    }

    strm.next_in = compressed.data(); 
    strm.avail_in = compressed.size(); 
    strm.next_out = decompressed.data();
    strm.avail_out = decompressed.size(); 

    lzma_code(&strm, LZMA_FINISH); 
    size_t totalOut = decompressed.size() - strm.avail_out; 
    lzma_end(&strm); 

    // convertir los bytes descomprimidos en string de texto 
    std::string text(decompressed.begin(), decompressed.begin() + totalOut); 

    // parsear frame por frame
    // cada frame tiene el formato: delta|keys|x|y
    // y los frames estan separados por comas 
    std::vector<KeyFrame> frames; 
    long long currentTime = 0; 

    std::stringstream ss(text); 
    std::string frameStr; 

    while (std::getline(ss, frameStr, ',')) {
        if (frameStr.empty()) continue; 

        //separa los 4 campos '|'
        std::stringstream ss2(frameStr); 
        std::string part; 
        std::vector<std::string> parts; 
        while (std::getline(ss2, part, '|')) {
            parts.push_back(part); 
        }
        if (parts.size() < 2) continue;

        long long delta = std::stoll(parts[0]); 
        int keys        = std::stoi(parts[1]); 

        //deltas negativos son frames especiales de metadata, se ignoran
        if (delta < 0) continue; 

        currentTime += delta; 
        frames.push_back({currentTime, keys});
    }

    return frames; 
}

ReplayData parseOsr(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb"); 
    if (!f) throw std::runtime_error("no se pudo abrir: " + path); 

    ReplayData data; 

    // modo de juego (1 byte), debe ser 3 para mania
    uint8_t mode; 
    fread(&mode, 1, 1, f); 
    if (mode != 3) throw std::runtime_error("el replay no es de mania"); 

    // version del cliente (4 bytes), no se necesita pero hay que leerla 
    // para avanzar la posicion en el archivo 

    uint32_t version; 
    fread(&version, 4, 1, f); 

    // string del header 
    data.beatmapHash = readOsuString(f); 
    data.playerName  = readOsuString(f);
    readOsuString(f); // hash del replay, no se necesita 
    
    //judgements (2 bytes cada uno)
    uint16_t v; 
    fread(&v, 2, 1, f); data.count300  = v; 
    fread(&v, 2, 1, f); data.count100  = v; 
    fread(&v, 2, 1, f); data.count50   = v; 
    fread(&v, 2, 1, f); data.count320  = v; 
    fread(&v, 2, 1, f); data.count200  = v; 
    fread(&v, 2, 1, f); data.countMiss = v; 

    // score y combo (4 y 2 bytes)
    uint32_t score;
    fread(&score, 4, 1, f);
    data.totalScore = score;

    fread(&v, 2, 1, f);
    data.maxCombo = v;

    // perfect flag (1 byte), no lo necesitamos
    uint8_t perfect;
    fread(&perfect, 1, 1, f);

    // mods (4 bytes), no los necesitamos por ahora
    uint32_t mods;
    fread(&mods, 4, 1, f);

    // barra de vida y timestamp,  no los necesitamos
    readOsuString(f);
    uint64_t timestamp;
    fread(&timestamp, 8, 1, f);

    // leer el bloque comprimido
    uint32_t compressedSize;
    fread(&compressedSize, 4, 1, f);

    std::vector<uint8_t> compressed(compressedSize);
    fread(compressed.data(), 1, compressedSize, f);

    fclose(f);

    // descomprimir y parsear los frames
    data.frames = parseFrames(compressed);

    return data;
}