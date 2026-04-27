#pragma once
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include "skinmanager/SkinManager.hpp"
#include <SFML/Graphics.hpp>

class Renderer {
public:
    Renderer(int width, int height);

    void setSkin(SkinManager& skin); 

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
    void drawBackground(); 
    void drawColumns(); 

    // target parameter lets both preview() and exportVideo() share same
    // draw calls, windows_ for preview, RenderTexture for export

    void drawNotes(const std::vector<ProcessedNote>& notes,
                   const ScrollCalculator& scroll, 
                   long long currentTime, 
                   sf::RenderTarget& target); 

    void drawKeys(int activeKeys, sf::RenderTarget& target); 

    void drawHUD(const std::vector<ProcessedNote>& notes,
                 long long currentTime, 
                 sf::RenderTarget& target); 
    
    sf::Color colorForJudgement(Judgement j) const; 

    sf::RenderWindow window_; 
    int width_, height_, hitY_, colWidth_; 

    sf::Font font_; 
    bool fontLoaded_ = false; 

    SkinManager* skin_ = nullptr; // non-owning pointer, set via setSkin()
}; 