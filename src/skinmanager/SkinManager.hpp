#pragma once 
#include <miniz/miniz.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <array>

// Loads osu!mania 4K skin textures from a .osk file (exported from osu!lazer)
// A .osk is a ZIP archive with the classic osu!stable skin structure inside
// Textures are loaded entirely in memory, no temp files written to disk
// Calls std::exit(1) if any required asset is missing 

class SkinManager {
    public: 
    // Loads all required textures from a .osk file
    // Expected filenames inside the archive (osu!stable naming convention):
    //   mania-note1.png  ... mania-note4.png        (normal notes)
    //   mania-note1H.png ... mania-note4H.png        (LN head)
    //   mania-note1L.png ... mania-note4L.png        (LN body)
    //   mania-note1T.png ... mania-note4T.png        (LN tail)
    //   mania-key1.png   ... mania-key4.png          (key up)
    //   mania-key1D.png  ... mania-key4D.png         (key down/pressed)
    void load(const std::string& oskPath);

    const sf::Texture& getNoteTexture(int col) const; 
    const sf::Texture& getLNHeadTexture(int col) const; 
    const sf::Texture& getLNBodyTexture(int col) const; 
    const sf::Texture& getLNTailTexture(int col) const; 

    // pressed=true returns the KeyDown texture, pressed=false returns KeyUp
    const sf::Texture& getKeyTexture(int col, bool pressed) const; 

    bool isLoaded() const { return loaded_; }

private:
    // Extracts a file by name form an open zip archive into a buffer
    // Returns an empty vector if the file is not found
    static std::vector<uint8_t> extractFileFromZip(mz_zip_archive& zip, const std::string& filename); 

    // Loads a texture from a raw PNG buffer
    // Exits with an error message if the buffer is empty or the load fails
    static sf::Texture textureFromBuffer(const std::vector<uint8_t>& buf, const std::string& assetName);

    // osu! skin uses 1-based column indices in filenames:
    // col 0 -> "1", col 1 -> "2", col 2 -> "3", col 3 -> "4"
     static std::string colStr(int col) { return std::to_string(col + 1); }

    std::array<sf::Texture, 2> noteTextures_;
    std::array<sf::Texture, 2> lnHeadTextures_;
    std::array<sf::Texture, 2> lnBodyTextures_;
    // no tail, use head flipped
    std::array<sf::Texture, 2> keyUpTextures_;
    std::array<sf::Texture, 2> keyDownTextures_;

    // 4K uses 2 unique textures mirrored: cols 0,3 -> idx 0 / cols 1,2 -> idx 1
    static int colToIdx(int col) { return ( col == 1 || col == 2) ? 1 : 0; }

    bool loaded_ = false; 
          
};