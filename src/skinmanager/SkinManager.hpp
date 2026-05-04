#pragma once 
#include <miniz/miniz.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <array>

// Loads osu!mania 4K skin textures from a .osk file (exported from osu!lazer)
// A .osk is a ZIP archive with the classic osu!stable skin structure inside
// Textures are loaded entirely in memory, no temp files written to disk
// Calls std::exit(1) if any required asset is missing 

// Parsed values from the [Mania] Keys: 4 block in Skin.ini 
struct ManiaSkinConfig {
    int columnStart = 136;          // default osu! 4K value
    int hitPosition = 402;          // default
    int scorePosition = 300; 
    int comboPosition = 275; 
    std::array<int, 4> columnWidths = {45, 45, 45, 45};  // default
    std::array<int, 5> columnLineWidths = {2, 2, 2, 2, 2};
    std::array<sf::Color, 4> colourLight = {
    sf::Color(102, 205, 107, 175),
    sf::Color(69,  188, 250, 175),
    sf::Color(69,  188, 250, 175),
    sf::Color(102, 205, 107, 175)
};

};

class SkinManager {
public: 
    void load(const std::string& oskPath);

    const sf::Texture& getNoteTexture(int col) const; 
    const sf::Texture& getLNHeadTexture(int col) const; 
    const sf::Texture& getLNBodyTexture(int col) const; 
    const sf::Texture& getLNTailTexture(int col) const; 
    const sf::Texture& getStageLeft()   const { return stageLeftTexture_;   }
    const sf::Texture& getStageRight()  const { return stageRightTexture_;  }
    const sf::Texture& getStageBottom() const { return stageBottomTexture_; }
    const sf::Texture& getStageHint()   const { return stageHintTexture_;   }
    const sf::Texture& getScoreDigit(int d) const { return scoreDigits_[d]; }
    const sf::Texture& getScoreX()       const { return scoreX_;       }
    const sf::Texture& getScorePercent() const { return scorePercent_; }
    const sf::Texture& getScoreDot()     const { return scoreDot_;     }
    const sf::Texture& getComboDigit(int d) const { return comboDigits_[d]; }
    const sf::Texture& getLightingN(int frame) const { return lightingNTextures_[frame]; }
    const sf::Texture& getLightingL(int frame) const { return lightingLTextures_[frame]; }
    static constexpr int LIGHTING_FRAMES = 12;
    const sf::Texture& getStageLight() const { return stageLightTexture_; }

    const ManiaSkinConfig& getConfig() const { return config_; }

    // pressed=true returns the KeyDown texture, pressed=false returns KeyUp
    const sf::Texture& getKeyTexture(int col, bool pressed) const; 
    // j: 0=miss, 1=50, 2=100, 3=200, 4=300, 5=320
    const sf::Texture& getHitTexture(int j) const { return hitTextures_[j]; }
    bool isLoaded() const { return loaded_; }

private:
    // Extracts a file by name form an open zip archive into a buffer
    // Returns an empty vector if the file is not found
    static std::vector<uint8_t> extractFileFromZip(mz_zip_archive& zip, const std::string& filename); 

    // Loads a texture from a raw PNG buffer
    // Exits with an error message if the buffer is empty or the load fails
    static sf::Texture textureFromBuffer(const std::vector<uint8_t>& buf, const std::string& assetName);
    static ManiaSkinConfig parseSkinIni(const std::string& content);

    // osu! skin uses 1-based column indices in filenames:
    // col 0 -> "1", col 1 -> "2", col 2 -> "3", col 3 -> "4"
     static std::string colStr(int col) { return std::to_string(col + 1); }

    std::array<sf::Texture, 2> noteTextures_;
    std::array<sf::Texture, 2> lnHeadTextures_;
    std::array<sf::Texture, 2> lnBodyTextures_;

    // no tail, use head flipped
    std::array<sf::Texture, 2> keyUpTextures_;
    std::array<sf::Texture, 2> keyDownTextures_;
    std::array<sf::Texture, 10> scoreDigits_; 
    std::array<sf::Texture, 10> comboDigits_;   
    std::array<sf::Texture, 12> lightingNTextures_;
    std::array<sf::Texture, 12> lightingLTextures_;

    sf::Texture stageLeftTexture_;
    sf::Texture stageRightTexture_;
    sf::Texture stageBottomTexture_; 
    sf::Texture stageHintTexture_;
    sf::Texture hitTextures_[6]; //0=miss, 50, 100, 200, 300, 320
    sf::Texture scoreX_; 
    sf::Texture scorePercent_; 
    sf::Texture scoreDot_;
    sf::Texture stageLightTexture_;
    

    // 4K uses 2 unique textures mirrored: cols 0,3 -> idx 0 / cols 1,2 -> idx 1
    static int colToIdx(int col) { return ( col == 1 || col == 2) ? 1 : 0; }

    ManiaSkinConfig config_;
    bool loaded_ = false; 
          
};