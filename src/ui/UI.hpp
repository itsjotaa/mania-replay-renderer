#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

// a simple clickable button 
struct Button {
    sf::RectangleShape rect;
    sf::Text label;
    bool hovered = false;

    Button(const sf::Font& font, const std::string& text, float x, float y, float w, float h);
    bool contains(sf::Vector2f point) const;
    void draw(sf::RenderWindow& window);
    void setHovered(bool h);
};

// simple text input field
struct TextInput {
    sf::RectangleShape rect;
    sf::Text text_;
    std::string value;
    bool focused = false;

    TextInput(const sf::Font& font, const std::string& defaultVal,
              float x, float y, float w, float h);
    bool contains(sf::Vector2f point) const;
    void handleKey(const sf::Event::TextEntered& e);
    void draw(sf::RenderWindow& window);
};

// main screen for file selection 
class UI {
public:
    UI(int width, int height);

    // shows the window and return true if the user confirmed
    // fills the paths with the selected files 
    bool run(std::string& osrPath, std::string& osuPath, std::string& oskPath, 
             int& outWidth, int& outHeight, int& outFps, double& outScroll);

private:
    void drawFileRow(sf::RenderWindow& window, const std::string& label,
                     const std::string& path, float y, Button& btn);

    sf::Font font_;
    int width_;
    int height_;

        // config inputs
    TextInput* inputWidth_  = nullptr;
    TextInput* inputHeight_ = nullptr;
    TextInput* inputFps_    = nullptr;
    TextInput* inputScroll_ = nullptr;
};