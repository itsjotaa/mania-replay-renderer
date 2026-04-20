#include "UI.hpp"
#include <nfd.hpp>
#include <SFML/Window/Event.hpp>
#include <iostream>

// colors
static const sf::Color COL_BG      = sf::Color(20, 20, 30);
static const sf::Color COL_PANEL   = sf::Color(35, 35, 50);
static const sf::Color COL_ACCENT  = sf::Color(100, 180, 255);
static const sf::Color COL_BTN     = sf::Color(50, 50, 70);
static const sf::Color COL_BTN_HOV = sf::Color(70, 70, 100);
static const sf::Color COL_TEXT    = sf::Color(220, 220, 220);
static const sf::Color COL_DIM     = sf::Color(120, 120, 140);

// --- Button ---

Button::Button(const sf::Font& font, const std::string& text,
               float x, float y, float w, float h)
    : label(font, text, 16)
{
    rect.setSize({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(COL_BTN);
    rect.setOutlineThickness(1);
    rect.setOutlineColor(COL_ACCENT);

    auto bounds = label.getLocalBounds();
    label.setPosition({
        x + w / 2 - bounds.size.x / 2,
        y + h / 2 - bounds.size.y / 2 - 2
    });
    label.setFillColor(COL_TEXT);
}

bool Button::contains(sf::Vector2f point) const {
    return rect.getGlobalBounds().contains(point);
}

void Button::setHovered(bool h) {
    hovered = h;
    rect.setFillColor(h ? COL_BTN_HOV : COL_BTN);
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(rect);
    window.draw(label);
}

// --- TextInput ---

TextInput::TextInput(const sf::Font& font, const std::string& defaultVal,
                     float x, float y, float w, float h)
    : text_(font, defaultVal, 16), value(defaultVal)
{
    rect.setSize({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(COL_PANEL);
    rect.setOutlineThickness(1);
    rect.setOutlineColor(sf::Color(60, 60, 80));

    text_.setFillColor(COL_TEXT);
    text_.setPosition({x + 8, y + 10});
}

bool TextInput::contains(sf::Vector2f point) const {
    return rect.getGlobalBounds().contains(point);
}

void TextInput::handleKey(const sf::Event::TextEntered& e) {
    if (!focused) return;

    // backspace
    if (e.unicode == 8 && !value.empty()) {
        value.pop_back();
    }
    // only accept digits and dot (for scroll speed)
    else if ((e.unicode >= '0' && e.unicode <= '9') || e.unicode == '.') {
        value += static_cast<char>(e.unicode);
    }

    text_.setString(value);
}

void TextInput::draw(sf::RenderWindow& window) {
    rect.setOutlineColor(focused ? COL_ACCENT : sf::Color(60, 60, 80));
    window.draw(rect);
    window.draw(text_);
}

// --- UI ---

UI::UI(int width, int height)
    : width_(width), height_(height)
{
    (void)font_.openFromFile("assets/font.ttf");
}

void UI::drawFileRow(sf::RenderWindow& window, const std::string& label,
                     const std::string& path, float y, Button& btn) {
    // left label
    sf::Text labelText(font_, label, 18);
    labelText.setFillColor(COL_ACCENT);
    labelText.setPosition({60, y + 12});
    window.draw(labelText);

    // path box
    sf::RectangleShape pathBox({460, 44});
    pathBox.setPosition({220, y});
    pathBox.setFillColor(COL_PANEL);
    pathBox.setOutlineThickness(1);
    pathBox.setOutlineColor(sf::Color(60, 60, 80));
    window.draw(pathBox);

    // truncated path text
    std::string display = path.empty() ? "not selected" : path;
    if (display.size() > 45) {
        display = "..." + display.substr(display.size() - 42);
    }
    sf::Text pathText(font_, display, 14);
    pathText.setFillColor(path.empty() ? COL_DIM : COL_TEXT);
    pathText.setPosition({228, y + 13});
    window.draw(pathText);

    btn.draw(window);
}

bool UI::run(std::string& osrPath, std::string& osuPath, std::string& audioPath,
             int& outWidth, int& outHeight, int& outFps, double& outScroll) {

    sf::RenderWindow window(
        sf::VideoMode({(unsigned int)width_, (unsigned int)height_}),
        "mania-replay-renderer"
    );
    window.setFramerateLimit(60);

    // initialize NFD
    NFD::Guard nfdGuard;

    // file buttons
    Button btnOsr   (font_, "Browse", 700, 160, 100, 44);
    Button btnOsu   (font_, "Browse", 700, 230, 100, 44);
    Button btnAudio (font_, "Browse", 700, 300, 100, 44);
    Button btnExport (font_, "Export",  460, 530, 140, 48);
    Button btnPreview(font_, "Preview", 310, 530, 140, 48);

    // config inputs
    TextInput inputWidth (font_, "1280", 220, 390, 80, 36);
    TextInput inputHeight(font_, "720",  320, 390, 80, 36);
    TextInput inputFps   (font_, "60",   220, 440, 80, 36);
    TextInput inputScroll(font_, "10",   220, 490, 80, 36);

    // list for focus management
    std::vector<TextInput*> inputs = {&inputWidth, &inputHeight, &inputFps, &inputScroll};

    while (window.isOpen()) {
        auto mousePixels = sf::Mouse::getPosition(window);
        auto windowSize  = window.getSize();
        sf::Vector2f mouse = {
            (float)mousePixels.x * width_  / (float)windowSize.x,
            (float)mousePixels.y * height_ / (float)windowSize.y
        };

        // update button hover
        btnOsr.setHovered(btnOsr.contains(mouse));
        btnOsu.setHovered(btnOsu.contains(mouse));
        btnAudio.setHovered(btnAudio.contains(mouse));
        btnExport.setHovered(btnExport.contains(mouse));
        btnPreview.setHovered(btnPreview.contains(mouse));

        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                sf::Vector2f pos = {
                    (float)click->position.x * width_  / (float)windowSize.x,
                    (float)click->position.y * height_ / (float)windowSize.y
                };

                // focus text inputs
                for (auto* input : inputs) {
                    input->focused = input->contains(pos);
                }

                // file dialogs
                NFD::UniquePath outPath;

                if (btnOsr.contains(pos)) {
                    nfdfilteritem_t filters[] = {{"osu! Replay", "osr"}};
                    if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY)
                        osrPath = outPath.get();
                }
                if (btnOsu.contains(pos)) {
                    nfdfilteritem_t filters[] = {{"osu! Beatmap / Map Set", "osu,osz" }};
                    if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY)
                        osuPath = outPath.get();
                }
                if (btnAudio.contains(pos)) {
                    nfdfilteritem_t filters[] = {{"Audio", "mp3,ogg,wav"}};
                    if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY)
                        audioPath = outPath.get();
                }
                if (btnPreview.contains(pos)) {
                    if (!osrPath.empty()) {
                        outWidth  = inputWidth.value.empty()  ? 1280 : std::stoi(inputWidth.value);
                        outHeight = inputHeight.value.empty() ? 720  : std::stoi(inputHeight.value);
                        outFps    = inputFps.value.empty()    ? 60   : std::stoi(inputFps.value);
                        outScroll = inputScroll.value.empty() ? 10.0 : std::stod(inputScroll.value);
                        window.close();
                        return false; // preview
                    }
                }
                if (btnExport.contains(pos)) {
                    if (!osrPath.empty()) {
                        outWidth  = inputWidth.value.empty()  ? 1280 : std::stoi(inputWidth.value);
                        outHeight = inputHeight.value.empty() ? 720  : std::stoi(inputHeight.value);
                        outFps    = inputFps.value.empty()    ? 60   : std::stoi(inputFps.value);
                        outScroll = inputScroll.value.empty() ? 10.0 : std::stod(inputScroll.value);
                        window.close();
                        return true; // export
                    }
                }
            }

            // text input handling
            if (const auto* te = event->getIf<sf::Event::TextEntered>()) {
                for (auto* input : inputs) {
                    input->handleKey(*te);
                }
            }
        }

        // draw
        window.clear(COL_BG);

        // title
        sf::Text title(font_, "mania-replay-renderer", 28);
        title.setFillColor(COL_ACCENT);
        title.setPosition({
            width_ / 2.f - title.getLocalBounds().size.x / 2.f,
            50.f
        });
        window.draw(title);

        // version
        sf::Text version(font_, "v0.4", 16);
        version.setFillColor(COL_DIM);
        version.setPosition({
            width_ / 2.f - version.getLocalBounds().size.x / 2.f,
            90.f
        });
        window.draw(version);

        // file rows
        drawFileRow(window, "Replay (.osr)",  osrPath,   160, btnOsr);
        drawFileRow(window, "Beatmap", osuPath,   230, btnOsu);
        drawFileRow(window, "Audio",          audioPath, 300, btnAudio);

        // separator
        sf::RectangleShape sep({(float)width_ - 120.f, 1.f});
        sep.setPosition({60.f, 355.f});
        sep.setFillColor(sf::Color(60, 60, 80));
        window.draw(sep);

        // config labels
        auto drawLabel = [&](const std::string& txt, float x, float y) {
            sf::Text t(font_, txt, 16);
            t.setFillColor(COL_DIM);
            t.setPosition({x, y});
            window.draw(t);
        };

        drawLabel("Resolution:", 60,  398);
        drawLabel("x",           308, 398);
        drawLabel("FPS:",        60,  448);
        drawLabel("Scroll:",     60,  498);

        inputWidth.draw(window);
        inputHeight.draw(window);
        inputFps.draw(window);
        inputScroll.draw(window);

        // separator
        sf::RectangleShape sep2({(float)width_ - 120.f, 1.f});
        sep2.setPosition({60.f, 520.f});
        sep2.setFillColor(sf::Color(60, 60, 80));
        window.draw(sep2);

        // action buttons
        btnPreview.draw(window);
        btnExport.draw(window);

        window.display();
    }

    return false;
}
