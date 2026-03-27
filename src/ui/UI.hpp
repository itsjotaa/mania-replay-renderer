#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

// un botón clickeable simple
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

// la pantalla principal de selección de archivos
class UI {
public:
    UI(int width, int height);

    // muestra la ventana y devuelve true si el usuario confirmó
    // llena los paths con los archivos seleccionados
    bool run(std::string& osrPath, std::string& osuPath, std::string& audioPath, 
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