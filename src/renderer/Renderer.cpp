#include <iostream>
#include "Renderer.hpp"
#include <SFML/Window/Event.hpp>
#include <chrono>
#include <thread>
#include "encoder/FFmpegPipe.hpp"
#include "skinmanager/SkinManager.hpp"
#include <algorithm>

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

void Renderer::setSkin(SkinManager& skin) {
    skin_ = &skin;

    const auto& cfg = skin.getConfig();

   // scale column width proportionally to screen height
    // osu! renders at ~600px base height
    float scaleY  = (float)height_ / 480.0f;
    hitY_         = (int)(cfg.hitPosition * scaleY);
    colWidth_     =(int)(cfg.columnWidths[0] * scaleY * 0.85f);

    int totalRealWidth = colWidth_ * 4;
    stageOffsetX_ = (width_ - totalRealWidth) / 2;
}

static void drawScaledSprite(sf::RenderTarget& target, 
                              const sf::Texture& tex, 
                              float x, float y, float destWidth, 
                              bool anchorBottom = false) {
    sf::Sprite sprite(tex); 
    auto texSize = tex.getSize(); 
    float scale = destWidth / (float)texSize.x;
    float height = texSize.y * scale; 
    sprite.setScale({scale, scale});
    sprite.setPosition({x, anchorBottom ? y - height : y});
    target.draw(sprite); 
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

// void Renderer::drawBackground() {
//    window_.clear(COL_BACKGROUND);
// }

void Renderer::drawColumns(sf::RenderTarget& target) {
    int totalWidth = colWidth_ * 4;
    int offsetX    = stageOffsetX_;
    int halfWidth  = totalWidth / 2;

    // stage-left: positioned to the left of the stage
    {
        sf::Sprite left(skin_->getStageLeft());
        auto size = skin_->getStageLeft().getSize();
        float scaleY  = (float)height_ / size.y;
        float scaledW = size.x * scaleY;
        left.setScale({scaleY, scaleY});
        // right edge of sprite aligns with left edge of stage
        left.setPosition({(float)offsetX - scaledW, 0.f});
        target.draw(left);
    }

    // stage-right: positioned to the right of the stage
    {
        sf::Sprite right(skin_->getStageRight());
        auto size = skin_->getStageRight().getSize();
        float scaleY  = (float)height_ / size.y;
        right.setScale({scaleY, scaleY});
        // left edge of sprite aligns with right edge of stage
        right.setPosition({(float)(offsetX + totalWidth), 0.f});
        target.draw(right);
    }


    // column dividers on top of the stage background
 //   for (int i = 0; i <= 4; i++) {
 //       sf::RectangleShape line({2.f, (float)height_});
 //       line.setPosition({(float)(offsetX + i * colWidth_), 0});
 //       line.setFillColor(COL_COLUMN_LINE);
 //       target.draw(line);
 //   }
}

void Renderer::drawStageBottom(sf::RenderTarget& target) {
    if (!skin_) return;  // safety check
    int totalWidth = colWidth_ * 4;
    sf::Sprite bottom(skin_->getStageBottom());
    auto size = skin_->getStageBottom().getSize();
    float scaleX  = (float)totalWidth / size.x;
    float scaleY  = scaleX;
    float scaledH = size.y * scaleY;
    bottom.setScale({scaleX, scaleY});
    bottom.setPosition({(float)stageOffsetX_, (float)height_ - scaledH});
    target.draw(bottom);
}

void Renderer::drawStageHint(sf::RenderTarget& target) {
    if (!skin_) return;
    int offsetX = stageOffsetX_;
    for (int col = 0; col < 4; col++) {
        sf::Sprite h(skin_->getStageHint());
        auto sz = skin_->getStageHint().getSize();
        float scaleX  = (float)colWidth_ / sz.x;
        float scaleY  = scaleX * 3.0f;
        float scaledH = sz.y * scaleY;
        h.setScale({scaleX, scaleY});
        // center of sprite at hitY_
        h.setPosition({(float)(offsetX + col * colWidth_), (float)hitY_ - scaledH / 4.2f});
        target.draw(h);
    }
}

void Renderer::drawNotes(
    const std::vector<ProcessedNote>& notes,
    const ScrollCalculator& scroll,
    long long currentTime, 
    sf::RenderTarget& target
) {
    int totalWidth = colWidth_ * 4;
    int offsetX    = stageOffsetX_;
    int noteMargin = 0;

    for (const auto& pn : notes) {
        if (!scroll.isVisible(pn.note.startTime, currentTime, hitY_, height_))
            continue;

        bool alreadyHit    = (pn.hitTime != -1 && pn.hitTime <= currentTime);
        bool alreadyMissed = (pn.judgement == Judgement::MISS &&
                              currentTime > pn.note.startTime + 161);
        if (alreadyHit || alreadyMissed) continue;

        int   col = pn.note.column;
        float x   = offsetX + col * colWidth_ + noteMargin;
        float w   = colWidth_ - noteMargin * 2;
        float y   = scroll.getNoteY(pn.note.startTime, currentTime, hitY_);

        if (!pn.note.isHold) {
            // normal note: anchor bottom edge to the note's Y position
            drawScaledSprite(target, skin_->getNoteTexture(col),
                             x, y, w, /*anchorBottom=*/true);
        } else {
            // --- long note: tail (top) → body (stretch) → head (bottom) ---
            // drawing order matters: body first so head/tail render on top

            float endY = scroll.getNoteY(pn.note.endTime, currentTime, hitY_);

            // measure head and tail heights so body fills the gap between them
            auto headSize = skin_->getLNHeadTexture(col).getSize();
            auto tailSize = skin_->getLNTailTexture(col).getSize();
            float headScale  = w / (float)headSize.x;
            float tailScale  = w / (float)tailSize.x;
            float headHeight = headSize.y * headScale;
            float tailHeight = tailSize.y * tailScale;

            float bodyTop    = endY + tailHeight;   // bottom edge of tail
            float bodyBottom = y   - headHeight;    // top edge of head
            float bodyHeight = bodyBottom - bodyTop;

            // body: stretched vertically to fill the gap
            if (bodyHeight > 0) {
                sf::Sprite body(skin_->getLNBodyTexture(col));
                auto bodySize = skin_->getLNBodyTexture(col).getSize();
                float scaleX  = w / (float)bodySize.x;
                float scaleY  = bodyHeight / (float)bodySize.y;
                body.setScale({scaleX, scaleY});
                body.setPosition({x, bodyTop});
                target.draw(body);
            }

            // tail: flipped vertically, reuses head texture
            {
                sf::Sprite tail(skin_->getLNTailTexture(col));
                float scaleX = w / (float)tailSize.x;
                tail.setScale({scaleX, -scaleX});
                tail.setPosition({x, endY + tailHeight});
                target.draw(tail);
            }
            // head: at the bottom of the LN (startTime position), anchor bottom
            drawScaledSprite(target, skin_->getLNHeadTexture(col),
                             x, y, w, /*anchorBottom=*/true);
        }
    }
}

void Renderer::drawKeys(int activeKeys, sf::RenderTarget& target) {
    int totalWidth = colWidth_ * 4;
    int offsetX    = stageOffsetX_;
    int keyMargin  = 4;

    for (int col = 0; col < 4; col++) {
        bool pressed = ((activeKeys >> col) & 1) != 0;

        float x = offsetX + col * colWidth_;  // sin margen
        float y = hitY_; 
        float w = (float)colWidth_;           // ancho completo de la columna
        float h = height_ - y;               // altura hasta el borde

        const sf::Texture& tex = skin_->getKeyTexture(col, pressed);
        auto texSize = tex.getSize();
        sf::Sprite key(tex);
        key.setScale({w / (float)texSize.x, h / (float)texSize.y});
        key.setPosition({x, y});
        target.draw(key);
    }
}

void Renderer::updateBursts(const std::vector<ProcessedNote>& notes, long long currentTime) {
    for (const auto& pn : notes) {
        // only care about notes that were just hit/missed this frame
        long long hitTime = (pn.hitTime != -1) ? pn.hitTime
                          : (pn.judgement == Judgement::MISS ? pn.note.startTime + 161 : -1);
        if (hitTime == -1) continue;

        // only spawn burst in a small window around the hit time (1 frame = ~16ms)
        if (hitTime < currentTime - 16 || hitTime > currentTime) continue;

        // avoid duplicate bursts for the same note
        bool alreadySpawned = false;
        for (const auto& b : activeBursts_) {
            if (b.col == pn.note.column && std::abs(b.spawnTime - hitTime) < 32) {
                alreadySpawned = true;
                break;
            }
        }
        if (alreadySpawned) continue;

        int j = 0;
        switch (pn.judgement) {
            case Judgement::MISS: j = 0; break;
            case Judgement::J50:  j = 1; break;
            case Judgement::J100: j = 2; break;
            case Judgement::J200: j = 3; break;
            case Judgement::J300: j = 4; break;
            case Judgement::J320: j = 5; break;
            default: continue;
        }

        activeBursts_.push_back({pn.note.column, j, hitTime});
    }

    // remove expired bursts
    activeBursts_.erase(
        std::remove_if(activeBursts_.begin(), activeBursts_.end(),
            [&](const HitBurst& b) {
                return currentTime - b.spawnTime > BURST_DURATION;
            }),
        activeBursts_.end()
    );
}

void Renderer::drawBursts(long long currentTime, sf::RenderTarget& target) {
    if (activeBursts_.empty()) return;

    // only draw the most recent burst
    const HitBurst& b = activeBursts_.back();

    const sf::Texture& tex = skin_->getHitTexture(b.judgement);
    auto texSize = tex.getSize();
    float targetH = 20.f; 
    float scale   = targetH / texSize.y; 
    float scaledW = texSize.x * scale;
    float scaledH = texSize.y * scale;

    float stageCenterX = stageOffsetX_ + (colWidth_ * 4) / 2.f;
    float x = stageCenterX - scaledW / 2.f;
    float y = hitY_ - 150.f;

    float progress = (float)(currentTime - b.spawnTime) / BURST_DURATION;
    uint8_t alpha  = (uint8_t)((1.0f - progress) * 255);

    sf::Sprite sprite(tex);
    sprite.setScale({scale, scale});
    sprite.setPosition({x, y});
    sprite.setColor(sf::Color(255, 255, 255, alpha));
    target.draw(sprite);
}

// draws a number using skin score digit sprites, centered at (cx, y)
void Renderer::drawSkinNumber(const std::string& text, float x, float y,
                               float digitH, sf::RenderTarget& target,
                               bool useCombo, TextAlign align) {
    // calculate total width
    float totalW = 0;
    for (char c : text) {
        const sf::Texture* tex = nullptr;
        if (c >= '0' && c <= '9')   tex = useCombo ? &skin_->getComboDigit(c - '0') : &skin_->getScoreDigit(c - '0');
        else if (c == 'x')          tex = &skin_->getScoreX();
        else if (c == '%')          tex = &skin_->getScorePercent();
        else if (c == '.')          tex = &skin_->getScoreDot();
        if (!tex) continue;
        float scale = digitH / tex->getSize().y;
        totalW += tex->getSize().x * scale;
    }

    // calculate start x based on alignment
    float startX = x;
    if (align == TextAlign::Center) startX = x - totalW / 2.f;
    else if (align == TextAlign::Right) startX = x - totalW;
    // Left: startX = x as-is

    for (char c : text) {
        const sf::Texture* tex = nullptr;
        if (c >= '0' && c <= '9')   tex = useCombo ? &skin_->getComboDigit(c - '0') : &skin_->getScoreDigit(c - '0');
        else if (c == 'x')          tex = &skin_->getScoreX();
        else if (c == '%')          tex = &skin_->getScorePercent();
        else if (c == '.')          tex = &skin_->getScoreDot();
        if (!tex) continue;
        float scale = digitH / tex->getSize().y;
        sf::Sprite s(*tex);
        s.setScale({scale, scale});
        s.setPosition({startX, y});
        target.draw(s);
        startX += tex->getSize().x * scale;
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
    int offsetX    = stageOffsetX_;

// score - top right corner
    drawSkinNumber(std::to_string(score), width_ - 20.f, 20, 36.f, target, 
                   false, TextAlign::Right);

// accuracy - below score
    char accBuf[16];
    snprintf(accBuf, sizeof(accBuf), "%.2f%%", acc);
    drawSkinNumber(accBuf, width_ - 20.f, 64, 24.f, target, 
                   false, TextAlign::Right);

// combo - center using skin digits
    float stageCenterX = stageOffsetX_ + (colWidth_ * 4) / 2.f;
    drawSkinNumber(std::to_string(combo), stageCenterX, 
                   (float)hitY_ - 140, 36.f, target, true);

    // judgement counters - top right
    auto drawJudge = [&](const std::string& label, int count, sf::Color color, float y) {
        sf::Text t(font_, label + ": " + std::to_string(count), 18);
        t.setFillColor(color);
        t.setPosition({(float)(offsetX + totalWidth + 10), y});
        target.draw(t);
    };

  //  drawJudge("320", j320, sf::Color(255, 220, 50),  20);
  //  drawJudge("300", j300, sf::Color(100, 200, 255), 44);
  //  drawJudge("200", j200, sf::Color(100, 255, 100), 68);
  //  drawJudge("100", j100, sf::Color(100, 100, 255), 92);
  //  drawJudge("50",  j50,  sf::Color(200, 100, 200), 116);
  //  drawJudge("Miss",miss, sf::Color(255, 60,  60),  140);
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
        window_.clear(sf::Color::Transparent);
        drawColumns(window_);
        drawKeys(activeKeys, window_);
        drawStageHint(window_); 
        drawStageBottom(window_); 
        updateBursts(notes, currentTime); 
        drawBursts(currentTime, window_); 
        drawNotes(notes, scroll, currentTime, window_);
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

        rt.clear(sf::Color::Transparent);
        drawColumns(rt);
        drawKeys(activeKeys, rt);
        drawStageHint(rt); 
        drawStageBottom(rt); 
        updateBursts(notes, currentTime); 
        drawBursts(currentTime, rt); 
        drawNotes(notes, scroll, currentTime, rt);
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
