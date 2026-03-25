#pragma once
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include <SFML/Graphics.hpp>

class Renderer {
public:
    Renderer(int width, int height);

    // reproduce el replay en una ventana en tiempo real
    void preview(
        const std::vector<ProcessedNote>& notes,
        const ScrollCalculator& scroll,
        const ReplayData& replay,
        const BeatmapData& beatmap
    );

    void exportVideo(
    const std::vector<ProcessedNote>& notes,
    const ScrollCalculator& scroll,
    const ReplayData& replay,
    const BeatmapData& beatmap,
    const std::string& outputPath,
    const std::string& audioPath = "", 
    int fps = 60
);

private:
    // funciones de dibujo
    void drawBackground();
    void drawColumns();
    void drawNotes(const std::vector<ProcessedNote>& notes,
                   const ScrollCalculator& scroll,
                   long long currentTime);
    void drawKeys(int activeKeys);

    sf::RenderWindow window_;
    int width_;
    int height_;
    int hitY_;       // Y de la línea de juicio
    int colWidth_;   // ancho de cada columna en píxeles

    // colores para cada judgement
    sf::Color colorForJudgement(Judgement j) const;

       void drawHUD(const std::vector<ProcessedNote>& notes, long long currentTime, sf::RenderTarget& target);

    sf::Font font_;
    bool fontLoaded_ = false;
};