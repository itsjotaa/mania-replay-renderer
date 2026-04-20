#include <iostream>
#include "Renderer.hpp"
#include <SFML/Window/Event.hpp>
#include <chrono>
#include <thread>
#include "encoder/FFmpegPipe.hpp"

// theme colours
static const sf::Color COL_BACKGROUND  = sf::Color(20, 20, 30);
static const sf::Color COL_COLUMN_LINE = sf::Color(60, 60, 80);
static const sf::Color COL_HIT_LINE    = sf::Color(255, 255, 255, 180);
static const sf::Color COL_NOTE_1      = sf::Color(100, 180, 255);  // columns 1 and 4
static const sf::Color COL_NOTE_2      = sf::Color(255, 255, 255);  // columns 2 and 3
static const sf::Color COL_KEY_ACTIVE  = sf::Color(255, 255, 150);
static const sf::Color COL_MISS        = sf::Color(255, 60, 60);

Renderer::Renderer(int width, int height)
    : width_(width)
    , height_(height)
    , hitY_(height - 80)   // judgement line at 80px from bottom
    , colWidth_(width / 6) // each column takes 1/6 of the width
    // window_ is initialized below because it needs parameters
{
    window_.create(
        sf::VideoMode({(unsigned int)width, (unsigned int)height}),
        "mania-replay-renderer"
    );
    window_.setFramerateLimit(60);
    fontLoaded_ = font_.openFromFile("assets/font.ttf");
}

// returns the color of a note based on its judgement
sf::Color Renderer::colorForJudgement(Judgement j) const {
    switch (j) {
        case Judgement::J320: return sf::Color(255, 220, 50);   // gold
        case Judgement::J300: return sf::Color(100, 200, 255);  // light blue
        case Judgement::J200: return sf::Color(100, 255, 100);  // green
        case Judgement::J100: return sf::Color(100, 100, 255);  // blue
        case Judgement::J50:  return sf::Color(200, 100, 200);  // purple
        case Judgement::MISS: return COL_MISS;                  // red
        default:              return sf::Color::White;
    }
}

// calculates the X center of a column
// the 4 columns are centered on screen
// example for width=1280, colWidth=213:
//   offset to center 4 columns = (1280 - 4*213) / 2 = 214
//   col=0 -> x = 214 + 0*213 + 213/2 = 320
//   col=1 -> x = 214 + 1*213 + 213/2 = 533
static int colCenterX(int col, int colWidth, int width) {
    int totalWidth = colWidth * 4;
    int offsetX    = (width - totalWidth) / 2;
    return offsetX + col * colWidth + colWidth / 2;
}

void Renderer::drawBackground() {
    window_.clear(COL_BACKGROUND);
}

void Renderer::drawColumns() {
    int totalWidth = colWidth_ * 4;
    int offsetX    = (width_ - totalWidth) / 2;

    // draw column background (slightly lighter than the main background)
    sf::RectangleShape colBg({(float)totalWidth, (float)height_});
    colBg.setPosition({(float)offsetX, 0});
    colBg.setFillColor(sf::Color(30, 30, 45));
    window_.draw(colBg);

    // draw divider lines between columns
    for (int i = 0; i <= 4; i++) {
        sf::RectangleShape line({2.f, (float)height_});
        line.setPosition({(float)(offsetX + i * colWidth_), 0});
        line.setFillColor(COL_COLUMN_LINE);
        window_.draw(line);
    }

    // draw the judgement line (where notes are judged)
    sf::RectangleShape hitLine({(float)totalWidth, 3.f});
    hitLine.setPosition({(float)offsetX, (float)hitY_});
    hitLine.setFillColor(COL_HIT_LINE);
    window_.draw(hitLine);
}

void Renderer::drawNotes(
    const std::vector<ProcessedNote>& notes,
    const ScrollCalculator& scroll,
    long long currentTime
) {
    int totalWidth = colWidth_ * 4;
    int offsetX    = (width_ - totalWidth) / 2;
    int noteHeight = 30;  // note height in pixels
    int noteMargin = 4;   // gap between column edge and note

    for (const auto& pn : notes) {
        // check if the note is visible in this frame
        if (!scroll.isVisible(pn.note.startTime, currentTime, hitY_, height_)) {
            continue;
        }

        float y = scroll.getNoteY(pn.note.startTime, currentTime, hitY_);

        // if already hit or missed, skip drawing
        bool alreadyHit = (pn.hitTime != -1 && pn.hitTime <= currentTime);
        bool alreadyMissed = (pn.judgement == Judgement::MISS &&
                              currentTime > pn.note.startTime + 161);

        if (alreadyHit || alreadyMissed) {
            continue;
        }

        // base color by column (matches default osu!mania skin)
        // columns 0 and 3 -> blue, columns 1 and 2 -> white
        sf::Color color = (pn.note.column == 0 || pn.note.column == 3)
                ? COL_NOTE_1
                : COL_NOTE_2;

        // calculate note X position
        float x = offsetX + pn.note.column * colWidth_ + noteMargin;
        float w = colWidth_ - noteMargin * 2;

        // draw note body
        sf::RectangleShape noteRect({w, (float)noteHeight});
        noteRect.setPosition({x, y - noteHeight});
        noteRect.setFillColor(color);

        // lighter border for depth effect
        noteRect.setOutlineThickness(2);
        noteRect.setOutlineColor(sf::Color(255, 255, 255, 80));
        window_.draw(noteRect);

        // if hold note, draw body up to endTime
        if (pn.note.isHold) {
            float endY = scroll.getNoteY(pn.note.endTime, currentTime, hitY_);
            float holdHeight = y - endY;
            if (holdHeight > 0) {
                sf::RectangleShape holdBody({w * 0.6f, holdHeight});
                holdBody.setPosition({x + w * 0.2f, endY});
                holdBody.setFillColor(sf::Color(color.r, color.g, color.b, 120));
                window_.draw(holdBody);
            }
        }
    }
}

void Renderer::drawKeys(int activeKeys) {
    int totalWidth = colWidth_ * 4;
    int offsetX    = (width_ - totalWidth) / 2;
    int keyHeight  = 60;
    int keyMargin  = 4;

    for (int col = 0; col < 4; col++) {
        bool active = ((activeKeys >> col) & 1) != 0;

        float x = offsetX + col * colWidth_ + keyMargin;
        float y = hitY_ + 3;  // just below the judgement line
        float w = colWidth_ - keyMargin * 2;

        sf::RectangleShape key({w, (float)keyHeight});
        key.setPosition({x, y});

        if (active) {
            key.setFillColor(COL_KEY_ACTIVE);
        } else {
            sf::Color base = (col == 0 || col == 3) ? COL_NOTE_1 : COL_NOTE_2;
            key.setFillColor(sf::Color(base.r, base.g, base.b, 80));
        }

        window_.draw(key);
    }
}

void Renderer::drawHUD(
    const std::vector<ProcessedNote>& notes,
    long long currentTime,
    sf::RenderTarget& target
) {
    if (!fontLoaded_) return;

    // calculate score and combo in real time
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int j320=0, j300=0, j200=0, j100=0, j50=0, miss=0;

    for (const auto& pn : notes) {
        // only count notes that have already passed their hit window
        if (pn.hitTime == -1 && currentTime <= pn.note.startTime + 161) continue;
        if (pn.hitTime != -1 && pn.hitTime > currentTime) continue;

        switch (pn.judgement) {
            case Judgement::J320: j320++; score += 320; combo++; break;
            case Judgement::J300: j300++; score += 300; combo++; break;
            case Judgement::J200: j200++; score += 200; combo++; break;
            case Judgement::J100: j100++; score += 100; combo++; break;
            case Judgement::J50:  j50++;  score += 50;  combo++; break;
            case Judgement::MISS: miss++; combo = 0;             break;
            default: break;
        }
        if (combo > maxCombo) maxCombo = combo;
    }

    // calculate accuracy
    int total = j320 + j300 + j200 + j100 + j50 + miss;
    float acc = total > 0
        ? (float)(j320*320 + j300*300 + j200*200 + j100*100 + j50*50)
          / (float)(total * 320) * 100.f
        : 100.f;

    int totalWidth = colWidth_ * 4;
    int offsetX    = (width_ - totalWidth) / 2;

    // score - top left
    sf::Text scoreText(font_, std::to_string(score), 28);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({(float)(offsetX - 10) - scoreText.getLocalBounds().size.x, 20});
    target.draw(scoreText);

    // accuracy - below score
    char accBuf[16];
    snprintf(accBuf, sizeof(accBuf), "%.2f%%", acc);
    sf::Text accText(font_, accBuf, 22);
    accText.setFillColor(sf::Color(180, 220, 255));
    accText.setPosition({(float)(offsetX - 10) - accText.getLocalBounds().size.x, 56});
    target.draw(accText);

    // combo - center bottom
    sf::Text comboText(font_, std::to_string(combo) + "x", 36);
    comboText.setFillColor(sf::Color(255, 220, 50));
    comboText.setPosition({
        (float)width_ / 2 - comboText.getLocalBounds().size.x / 2,
        (float)hitY_ - 80
    });
    target.draw(comboText);

    // judgement counters - top right
    auto drawJudge = [&](const std::string& label, int count, sf::Color color, float y) {
        sf::Text t(font_, label + ": " + std::to_string(count), 18);
        t.setFillColor(color);
        t.setPosition({(float)(offsetX + totalWidth + 10), y});
        target.draw(t);
    };

    drawJudge("320", j320, sf::Color(255, 220, 50),  20);
    drawJudge("300", j300, sf::Color(100, 200, 255), 44);
    drawJudge("200", j200, sf::Color(100, 255, 100), 68);
    drawJudge("100", j100, sf::Color(100, 100, 255), 92);
    drawJudge("50",  j50,  sf::Color(200, 100, 200), 116);
    drawJudge("Miss",miss, sf::Color(255, 60,  60),  140);
}

void Renderer::preview(
    const std::vector<ProcessedNote>& notes,
    const ScrollCalculator& scroll,
    const ReplayData& replay,
    const BeatmapData& beatmap
) {
    // calculate total replay duration
    long long lastNoteTime = 0;
    for (const auto& pn : notes) {
        if (pn.note.endTime > lastNoteTime) lastNoteTime = pn.note.endTime;
    }
    long long totalDuration = lastNoteTime + 3000;

    // real start time
    auto startWall = std::chrono::steady_clock::now();
    long long startOffset = notes.empty() ? 0 : notes[0].note.startTime - 2000;

    // back button
    sf::RectangleShape backBtn({120.f, 36.f});
    backBtn.setPosition({10.f, 10.f});
    backBtn.setFillColor(sf::Color(50, 50, 70));
    backBtn.setOutlineThickness(1);
    backBtn.setOutlineColor(sf::Color(100, 180, 255));

    sf::Text backLabel(font_, "< Back", 16);
    backLabel.setFillColor(sf::Color(220, 220, 220));
    backLabel.setPosition({18.f, 16.f});

    while (window_.isOpen()) {
        // calculate current replay time
        auto now = std::chrono::steady_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                            (now - startWall).count();
        long long currentTime = startOffset + elapsed;

        // handle window events (close with X or Escape)
        while (auto event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window_.close();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) {
                    window_.close();
                }
            }
            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                sf::Vector2f pos = {(float)click->position.x, (float)click->position.y};
                if (backBtn.getGlobalBounds().contains(pos)) {
                    window_.close();
                }
            }
        }

        // find key state at current time
        int activeKeys = 0;
        for (const auto& frame : replay.frames) {
            if (frame.timestamp <= currentTime) {
                activeKeys = frame.keys;
            } else {
                break;
            }
        }

        // draw everything
        drawBackground();
        drawColumns();
        drawNotes(notes, scroll, currentTime);
        drawKeys(activeKeys);
        drawHUD(notes, currentTime, window_);

        window_.draw(backBtn);
        window_.draw(backLabel);

        window_.display();

        // stop when replay ends
        if (currentTime > totalDuration) {
            window_.close();
        }
    }
}

void Renderer::exportVideo(
    const std::vector<ProcessedNote>& notes,
    const ScrollCalculator& scroll,
    const ReplayData& replay,
    const BeatmapData& beatmap,
    const std::string& outputPath,
    const std::string& audioPath,
    int fps
) {
    // calculate total duration
    long long lastNoteTime = 0;
    for (const auto& pn : notes) {
        if (pn.note.endTime > lastNoteTime) lastNoteTime = pn.note.endTime;
    }
    long long totalDuration = lastNoteTime + 3000;
    long long startOffset   = notes.empty() ? 0 : notes[0].note.startTime - 2000;
    // RenderTexture draws in memory without showing a window
    sf::RenderTexture rt({(unsigned int)width_, (unsigned int)height_});

    FFmpegPipe encoder(outputPath, width_, height_, fps, audioPath, (double)startOffset);

    double frameMs = 1000.0 / fps;
    long long totalFrames = (long long)((totalDuration - startOffset) / frameMs);
    long long frameCount  = 0;

    std::cout << "Exporting " << totalFrames << " frames...\n";

    for (double t = startOffset; t < totalDuration; t += frameMs) {
        long long currentTime = (long long)t;

        // find key state at current time
        int activeKeys = 0;
        for (const auto& frame : replay.frames) {
            if (frame.timestamp <= currentTime) activeKeys = frame.keys;
            else break;
        }
        // ADD THIS:
    static bool firstKeyPrinted = false;
    if (!firstKeyPrinted && activeKeys != 0) {
        firstKeyPrinted = true;
    }
    static bool firstNotePrinted = false;
    if (!firstNotePrinted) {
        for (const auto& pn : notes) {
            if (scroll.isVisible(pn.note.startTime, currentTime, hitY_, height_)) {
                firstNotePrinted = true;
                break;
            }
        }
    }

        // draw to offscreen texture
        rt.clear(COL_BACKGROUND);

        int totalWidth = colWidth_ * 4;
        int offsetX    = (width_ - totalWidth) / 2;

        // column background
        sf::RectangleShape colBg({(float)totalWidth, (float)height_});
        colBg.setPosition({(float)offsetX, 0});
        colBg.setFillColor(sf::Color(30, 30, 45));
        rt.draw(colBg);

        // column dividers
        for (int i = 0; i <= 4; i++) {
            sf::RectangleShape line({2.f, (float)height_});
            line.setPosition({(float)(offsetX + i * colWidth_), 0});
            line.setFillColor(COL_COLUMN_LINE);
            rt.draw(line);
        }

        // judgement line
        sf::RectangleShape hitLine({(float)totalWidth, 3.f});
        hitLine.setPosition({(float)offsetX, (float)hitY_});
        hitLine.setFillColor(COL_HIT_LINE);
        rt.draw(hitLine);

        // notes
        int noteHeight = 30;
        int noteMargin = 4;
        for (const auto& pn : notes) {
            if (!scroll.isVisible(pn.note.startTime, currentTime, hitY_, height_)) continue;

            bool alreadyHit    = (pn.hitTime != -1 && pn.hitTime <= currentTime);
            bool alreadyMissed = (pn.judgement == Judgement::MISS &&
                                   currentTime > pn.note.startTime + 161);
            if (alreadyHit || alreadyMissed) continue;

            sf::Color color = (pn.note.column == 0 || pn.note.column == 3)
                              ? COL_NOTE_1 : COL_NOTE_2;

            float y = scroll.getNoteY(pn.note.startTime, currentTime, hitY_);
            float x = offsetX + pn.note.column * colWidth_ + noteMargin;
            float w = colWidth_ - noteMargin * 2;

            sf::RectangleShape noteRect({w, (float)noteHeight});
            noteRect.setPosition({x, y - noteHeight});
            noteRect.setFillColor(color);
            noteRect.setOutlineThickness(2);
            noteRect.setOutlineColor(sf::Color(255, 255, 255, 80));
            rt.draw(noteRect);

            if (pn.note.isHold) {
                float endY = scroll.getNoteY(pn.note.endTime, currentTime, hitY_);
                float holdHeight = y - endY;
                if (holdHeight > 0) {
                    sf::RectangleShape holdBody({w * 0.6f, holdHeight});
                    holdBody.setPosition({x + w * 0.2f, endY});
                    holdBody.setFillColor(sf::Color(color.r, color.g, color.b, 120));
                    rt.draw(holdBody);
                }
            }
        }

        // keys
        int keyHeight = 60;
        int keyMargin = 4;
        for (int col = 0; col < 4; col++) {
            bool active = ((activeKeys >> col) & 1) != 0;
            float x = offsetX + col * colWidth_ + keyMargin;
            float w = colWidth_ - keyMargin * 2;
            sf::RectangleShape key({w, (float)keyHeight});
            key.setPosition({x, (float)hitY_ + 3});
            if (active) {
                key.setFillColor(COL_KEY_ACTIVE);
            } else {
                sf::Color base = (col == 0 || col == 3) ? COL_NOTE_1 : COL_NOTE_2;
                key.setFillColor(sf::Color(base.r, base.g, base.b, 80));
            }
            rt.draw(key);
        }

        drawHUD(notes, currentTime, rt);
        rt.display();

        // capture frame and send to ffmpeg
        sf::Image frame = rt.getTexture().copyToImage();
        const uint8_t* pixels = frame.getPixelsPtr();
        encoder.writeFrame(pixels, width_ * height_ * 4);

        // print progress every 100 frames
        frameCount++;
        if (frameCount % 100 == 0) {
            int pct = (int)(frameCount * 100 / totalFrames);
            std::cout << "\r" << pct << "% (" << frameCount << "/" << totalFrames << " frames)" << std::flush;
        }
    }

    std::cout << "\nDone! Video saved to: " << outputPath << "\n";
}
