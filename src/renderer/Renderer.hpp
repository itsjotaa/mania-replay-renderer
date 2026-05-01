#pragma once
#include "parser/OsrParser.hpp"
#include "parser/OsuParser.hpp"
#include "engine/ReplayProcessor.hpp"
#include "engine/ScrollCalculator.hpp"
#include "skinmanager/SkinManager.hpp"
#include <SFML/Graphics.hpp>
#include <vector> 

struct HitBurst {
    int col;           // which column (0-3)
    int judgement;     // 0=miss, 1=50, 2=100, 3=200, 4=300, 5=320
    long long spawnTime; // when it appeared (ms)
};

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
    void drawColumns(sf::RenderTarget& target); 
    void drawStageHint(sf::RenderTarget& target);
    void drawStageBottom(sf::RenderTarget& target);

    // target parameter lets both preview() and exportVideo() share same
    // draw calls, windows_ for preview, RenderTexture for export

    void drawNotes(const std::vector<ProcessedNote>& notes,
                   const ScrollCalculator& scroll, 
                   long long currentTime, 
                   sf::RenderTarget& target); 

    void drawKeys(int activeKeys, sf::RenderTarget& target); 

    enum class TextAlign { Left, Center, Right}; 

    void drawSkinNumber(const std::string& text, float x, float y,
                    float digitH, sf::RenderTarget& target, 
                    bool useCombo = false,
                    TextAlign align = TextAlign::Center);

    void drawHUD(const std::vector<ProcessedNote>& notes,
                 long long currentTime, 
                 sf::RenderTarget& target); 
    void updateBursts(const std::vector<ProcessedNote>& notes, long long currentTime);
    void drawBursts(long long currentTime, sf::RenderTarget& target);
    
    std::vector<HitBurst> activeBursts_;
    static constexpr long long BURST_DURATION = 600; // ms
    
    sf::Color colorForJudgement(Judgement j) const; 

    sf::RenderWindow window_; 
    int width_, height_, hitY_, colWidth_; 
    int stageOffsetX_ = 0;

    sf::Font font_; 
    bool fontLoaded_ = false; 

    SkinManager* skin_ = nullptr; // non-owning pointer, set via setSkin()
}; 