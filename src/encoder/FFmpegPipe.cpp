#include "FFmpegPipe.hpp"
#include <stdexcept>
#include <cstdio> 
#include <iostream>

// Windows compatibility 
#ifdef _WIN32
    #define popen _popen 
    #define pclose _pclose 
#endif

FFmpegPipe::FFmpegPipe(
    const std::string& outputPath,
    int width,
    int height,
    int fps,
    const std::string& audioPath,
    double audioOffsetMs
) : pipe_(nullptr), width_(width), height_(height) {

        std::string audioInput = "";
    std::string audioMap   = "";

    if (!audioPath.empty()) {
        if (audioOffsetMs > 0) { 
            //audio starts later than video: seek into audio file
            double offsetSec = audioOffsetMs / 1000.0; 
            audioInput = "-ss " + std::to_string(offsetSec) + " -i " + audioPath + " "; 
        } else if (audioOffsetMs < 0) { 
            // video starts before audio: delay the audio
            long long delayMS = (long long)(-audioOffsetMs); 
            audioInput = "-i " + audioPath + " "; 
            audioMap = "-map 0:v -map 1:a -af 'adelay=" + std::to_string(delayMS) + "|" + std::to_string(delayMS) + "' -c:a aac ";
        } else {
            audioInput = "-i " + audioPath + " "; 
            audioMap = "-map 0:v -map 1:a -c:a aac "; 
            }
        }
    // build the ffmpeg command
    // -f rawvideo        → input is raw video without compression
    // -pixel_format rgba → each pixel are 4 bytes: R, G, B, A 
    // -video_size        → video resolution 
    // -framerate         → frames per second
    // -i pipe:0          → read from stdin
    // -c:v libx264       → compress with H.264
    // -pix_fmt yuv420p   → compatible format with most of media players 
    // -preset fast       → balance between speed and compress 
    std::string cmd =
        "ffmpeg -y "
        "-f rawvideo "
        "-pixel_format rgba "
        "-video_size " + std::to_string(width) + "x" + std::to_string(height) +
        " -framerate " + std::to_string(fps) +
        " -i pipe:0 "
        + audioInput + 
        "-c:v libx264 "
        "-pix_fmt yuv420p "
        "-preset fast "
        + audioMap
        + outputPath;

    pipe_ = popen(cmd.c_str(), "w");
    if (!pipe_) {
        throw std::runtime_error("could not open ffmpeg pipe");
    }
}


FFmpegPipe::~FFmpegPipe() {
    if (pipe_) {
        pclose(pipe_);
        pipe_ = nullptr;
    }
}

void FFmpegPipe::writeFrame(const uint8_t* pixels, size_t size) {
    if (!pipe_) return;
    fwrite(pixels, 1, size, pipe_);
}