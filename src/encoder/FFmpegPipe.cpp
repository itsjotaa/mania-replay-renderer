#include "FFmpegPipe.hpp"
#include <stdexcept>

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
        // -ss indica desde qué segundo empezar el audio
        // si el video empieza antes que las notas, hay que saltar ese tiempo
        double offsetSec = audioOffsetMs / 1000.0;
        audioInput = "-ss " + std::to_string(offsetSec) + " -i " + audioPath + " ";
        audioMap   = "-map 0:v -map 1:a -c:a aac ";
    }

    // construir el comando ffmpeg
    // -f rawvideo        → el input es video crudo sin comprimir
    // -pixel_format rgba → cada pixel son 4 bytes: R, G, B, A
    // -video_size        → resolución del video
    // -framerate         → cuadros por segundo
    // -i pipe:0          → leer desde stdin
    // -c:v libx264       → comprimir con H.264
    // -pix_fmt yuv420p   → formato compatible con la mayoría de reproductores
    // -preset fast       → balance entre velocidad y compresión
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

    printf("FFmpeg cmd: %s\n", cmd.c_str());
    pipe_ = popen(cmd.c_str(), "w");
    if (!pipe_) {
        throw std::runtime_error("no se pudo abrir el pipe a ffmpeg");
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