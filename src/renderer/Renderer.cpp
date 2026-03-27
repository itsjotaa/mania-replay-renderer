#include <iostream>
#include "Renderer.hpp"
#include <SFML/Window/Event.hpp>
#include <chrono>
#include <thread>
#include "encoder/FFmpegPipe.hpp"

// colores del tema
static const sf::Color COL_BACKGROUND  = sf::Color(20, 20, 30);
static const sf::Color COL_COLUMN_LINE = sf::Color(60, 60, 80);
static const sf::Color COL_HIT_LINE    = sf::Color(255, 255, 255, 180);
static const sf::Color COL_NOTE_1      = sf::Color(100, 180, 255);  // columnas 1 y 4
static const sf::Color COL_NOTE_2      = sf::Color(255, 255, 255);  // columnas 2 y 3
static const sf::Color COL_KEY_ACTIVE  = sf::Color(255, 255, 150);
static const sf::Color COL_MISS        = sf::Color(255, 60, 60);

Renderer::Renderer(int width, int height)
    : width_(width)
    , height_(height)
    , hitY_(height - 80)   // línea de juicio a 80px del fondo
    , colWidth_(width / 6) // cada columna ocupa 1/6 del ancho
    // window_ se inicializa abajo porque necesita parámetros
{
    window_.create(
        sf::VideoMode({(unsigned int)width, (unsigned int)height}),
        "mania-replay-renderer"
    );
    window_.setFramerateLimit(60);
    fontLoaded_ = font_.openFromFile("assets/font.ttf");
}

// devuelve el color de una nota según su judgement
sf::Color Renderer::colorForJudgement(Judgement j) const {
    switch (j) {
        case Judgement::J320: return sf::Color(255, 220, 50);   // dorado
        case Judgement::J300: return sf::Color(100, 200, 255);  // celeste
        case Judgement::J200: return sf::Color(100, 255, 100);  // verde
        case Judgement::J100: return sf::Color(100, 100, 255);  // azul
        case Judgement::J50:  return sf::Color(200, 100, 200);  // violeta
        case Judgement::MISS: return COL_MISS;                  // rojo
        default:              return sf::Color::White;
    }
}

// calcula el X del centro de una columna
// las 4 columnas están centradas en la pantalla
// ejemplo para width=1280, colWidth=213:
//   offset para centrar las 4 columnas = (1280 - 4*213) / 2 = 214
//   col=0 → x = 214 + 0*213 + 213/2 = 320
//   col=1 → x = 214 + 1*213 + 213/2 = 533
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

    // dibujar el fondo de las columnas (un poco más claro que el fondo)
    sf::RectangleShape colBg({(float)totalWidth, (float)height_});
    colBg.setPosition({(float)offsetX, 0});
    colBg.setFillColor(sf::Color(30, 30, 45));
    window_.draw(colBg);

    // dibujar líneas divisoras entre columnas
    for (int i = 0; i <= 4; i++) {
        sf::RectangleShape line({2.f, (float)height_});
        line.setPosition({(float)(offsetX + i * colWidth_), 0});
        line.setFillColor(COL_COLUMN_LINE);
        window_.draw(line);
    }

    // dibujar la línea de juicio (donde se juzgan las notas)
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
    int noteHeight = 30;  // altura de cada nota en píxeles
    int noteMargin = 4;   // espacio entre el borde de la columna y la nota

    for (const auto& pn : notes) {
        // verificar si la nota es visible en este frame
        if (!scroll.isVisible(pn.note.startTime, currentTime, hitY_, height_)) {
            continue;
        }

        float y = scroll.getNoteY(pn.note.startTime, currentTime, hitY_);

        // determinar color: si ya fue hit, usar el color del judgement
        // si todavía no llegó su tiempo, usar el color base de la columna
        sf::Color color;
        bool alreadyHit = (pn.hitTime != -1 && pn.hitTime <= currentTime);
        bool alreadyMissed = (pn.judgement == Judgement::MISS &&
                              currentTime > pn.note.startTime + 161);

        if (alreadyHit || alreadyMissed) {
            continue;  // no dibujar notas que ya pasaron
        } else {
            // color base según columna (igual que osu!mania por defecto)
            // columnas 0 y 3 → azul, columnas 1 y 2 → blanco
            color = (pn.note.column == 0 || pn.note.column == 3)
                    ? COL_NOTE_1
                    : COL_NOTE_2;
        }

        // calcular posición X de la nota
        float x = offsetX + pn.note.column * colWidth_ + noteMargin;
        float w = colWidth_ - noteMargin * 2;

        // dibujar la nota
        sf::RectangleShape noteRect({w, (float)noteHeight});
        noteRect.setPosition({x, y - noteHeight});
        noteRect.setFillColor(color);

        // borde más claro para darle profundidad
        noteRect.setOutlineThickness(2);
        noteRect.setOutlineColor(sf::Color(255, 255, 255, 80));
        window_.draw(noteRect);

        // si es hold note, dibujar el cuerpo hasta el endTime
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
        float y = hitY_ + 3;  // justo debajo de la línea de juicio
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

    // calcular score y combo en tiempo real
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int j320=0, j300=0, j200=0, j100=0, j50=0, miss=0;

    for (const auto& pn : notes) {
        // solo contar notas que ya pasaron su tiempo de hit
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

    // calcular accuracy
    int total = j320 + j300 + j200 + j100 + j50 + miss;
    float acc = total > 0
        ? (float)(j320*320 + j300*300 + j200*200 + j100*100 + j50*50)
          / (float)(total * 320) * 100.f
        : 100.f;

    int totalWidth = colWidth_ * 4;
    int offsetX    = (width_ - totalWidth) / 2;

    // score — arriba a la izquierda
    sf::Text scoreText(font_, std::to_string(score), 28);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({(float)(offsetX - 10) - scoreText.getLocalBounds().size.x, 20});
    target.draw(scoreText);

    // accuracy — debajo del score
    char accBuf[16];
    snprintf(accBuf, sizeof(accBuf), "%.2f%%", acc);
    sf::Text accText(font_, accBuf, 22);
    accText.setFillColor(sf::Color(180, 220, 255));
    accText.setPosition({(float)(offsetX - 10) - accText.getLocalBounds().size.x, 56});
    target.draw(accText);

    // combo — abajo en el centro
    sf::Text comboText(font_, std::to_string(combo) + "x", 36);
    comboText.setFillColor(sf::Color(255, 220, 50));
    comboText.setPosition({
        (float)width_ / 2 - comboText.getLocalBounds().size.x / 2,
        (float)hitY_ - 80
    });
    target.draw(comboText);

    // judgements — arriba a la derecha
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
    // calcular la duración total del replay
    long long lastNoteTime = 0;
    for (const auto& pn : notes) {
        if (pn.note.endTime > lastNoteTime) lastNoteTime = pn.note.endTime;
    }
    long long totalDuration = lastNoteTime + 3000;

    // tiempo de inicio real
    auto startWall = std::chrono::steady_clock::now();
    long long startOffset = notes.empty() ? 0 : notes[0].note.startTime - 2000;

    // back button for preview
    sf::RectangleShape backBtn({120.f, 36.f});
    backBtn.setPosition({10.f, 10.f});
    backBtn.setFillColor(sf::Color(50, 50, 70));
    backBtn.setOutlineThickness(1);
    backBtn.setOutlineColor(sf::Color(100, 180, 255));

    sf::Text backLabel(font_, "< Back", 16);
    backLabel.setFillColor(sf::Color(220, 220, 220));
    backLabel.setPosition({18.f, 16.f});

    while (window_.isOpen()) {
        // calcular el tiempo actual del replay
        auto now = std::chrono::steady_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                            (now - startWall).count();
        long long currentTime = startOffset + elapsed;

        // manejar eventos de la ventana (cerrar con la X o con Escape)
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

        // encontrar el estado de teclas en el momento actual
        int activeKeys = 0;
        for (const auto& frame : replay.frames) {
            if (frame.timestamp <= currentTime) {
                activeKeys = frame.keys;
            } else {
                break;
            }
        }

        // dibujar todo
        drawBackground();
        drawColumns();
        drawNotes(notes, scroll, currentTime);
        drawKeys(activeKeys);
        drawHUD(notes, currentTime, window_); 
        window_.display();

        // terminar cuando el replay termine
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
    // calcular duración total
    long long lastNoteTime = 0;
    for (const auto& pn : notes) {
        if (pn.note.endTime > lastNoteTime) lastNoteTime = pn.note.endTime;
    }
    long long totalDuration = lastNoteTime + 3000;
    long long startOffset   = notes.empty() ? 0 : notes[0].note.startTime - 2000;

      // RenderTexture dibuja en memoria sin mostrar ventana
    sf::RenderTexture rt({(unsigned int)width_, (unsigned int)height_});

    FFmpegPipe encoder(outputPath, width_, height_, fps, audioPath, (double)startOffset);

    double frameMs = 1000.0 / fps;
    long long totalFrames = (long long)((totalDuration - startOffset) / frameMs);
    long long frameCount  = 0;

    std::cout << "Exportando " << totalFrames << " frames...\n";

    for (double t = startOffset; t < totalDuration; t += frameMs) {
        long long currentTime = (long long)t;

        // encontrar estado de teclas en este momento
        int activeKeys = 0;
        for (const auto& frame : replay.frames) {
            if (frame.timestamp <= currentTime) activeKeys = frame.keys;
            else break;
        }

        // dibujar en la textura offscreen
        rt.clear(COL_BACKGROUND);

        // redibujar usando rt en lugar de window_
        // necesitamos versiones que acepten RenderTarget
        // por ahora usamos un truco: swap temporal
        // (lo refactorizamos después si hace falta)

        // dibujar fondo de columnas
        int totalWidth = colWidth_ * 4;
        int offsetX    = (width_ - totalWidth) / 2;

        sf::RectangleShape colBg({(float)totalWidth, (float)height_});
        colBg.setPosition({(float)offsetX, 0});
        colBg.setFillColor(sf::Color(30, 30, 45));
        rt.draw(colBg);

        // líneas divisoras
        for (int i = 0; i <= 4; i++) {
            sf::RectangleShape line({2.f, (float)height_});
            line.setPosition({(float)(offsetX + i * colWidth_), 0});
            line.setFillColor(COL_COLUMN_LINE);
            rt.draw(line);
        }

        // línea de juicio
        sf::RectangleShape hitLine({(float)totalWidth, 3.f});
        hitLine.setPosition({(float)offsetX, (float)hitY_});
        hitLine.setFillColor(COL_HIT_LINE);
        rt.draw(hitLine);

        // notas
        int noteHeight = 30;
        int noteMargin = 4;
        for (const auto& pn : notes) {
            if (!scroll.isVisible(pn.note.startTime, currentTime, hitY_, height_)) continue;

            bool alreadyHit     = (pn.hitTime != -1 && pn.hitTime <= currentTime);
            bool alreadyMissed  = (pn.judgement == Judgement::MISS &&
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

        // teclas
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

        // capturar el frame y mandarlo a ffmpeg
        sf::Image frame = rt.getTexture().copyToImage();
        const uint8_t* pixels = frame.getPixelsPtr();
        encoder.writeFrame(pixels, width_ * height_ * 4);

        // mostrar progreso cada 100 frames
        frameCount++;
        if (frameCount % 100 == 0) {
            int pct = (int)(frameCount * 100 / totalFrames);
            std::cout << "\r" << pct << "% (" << frameCount << "/" << totalFrames << " frames)" << std::flush;
        }
    }

    std::cout << "\n¡Listo! Video guardado en: " << outputPath << "\n";
}