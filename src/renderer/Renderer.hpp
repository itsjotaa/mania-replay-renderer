#pragma once
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include <SFML/Graphics.hpp>

class Renderer {
public:
    Renderer(int width, int height);

    // reproduces the replay in a real time window
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
    // draw functions
    void drawBackground();
    void drawColumns();
    void drawNotes(const std::vector<ProcessedNote>& notes,
                   const ScrollCalculator& scroll,
                   long long currentTime);
    void drawKeys(int activeKeys);

    sf::RenderWindow window_;
    int width_;
    int height_;
    int hitY_;       // Y coordinate of the judgement line
    int colWidth_;   // width of each column in pixels

    // colours for each judgement 
    sf::Color colorForJudgement(Judgement j) const;

       void drawHUD(const std::vector<ProcessedNote>& notes, long long currentTime, sf::RenderTarget& target);

    sf::Font font_;
    bool fontLoaded_ = false;
};