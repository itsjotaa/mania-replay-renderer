#pragma once
#include <string>
#include <cstdint>
#include <cstdio>

class FFmpegPipe {
public:
    FFmpegPipe(const std::string& outputPath, int width, int height, int fps, 
        const std::string& audioPath = "", double audioOffsetMs = 0.0);
    ~FFmpegPipe();

    // escribe un frame raw RGBA al pipe de ffmpeg
    void writeFrame(const uint8_t* pixels, size_t size);

    bool isOpen() const { return pipe_ != nullptr; }

private:
    FILE* pipe_;
    int width_;
    int height_;
};