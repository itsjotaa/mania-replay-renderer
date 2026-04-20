# mania-replay-renderer

A tool that renders osu!mania 4K replays into MP4 videos with audio.  
Built from scratch in C++ without using osu!'s source code — parses `.osr` and `.osu` files directly, reconstructs gameplay frame by frame, and exports via FFmpeg.

![Demo](assets/images/demo.gif)

## Features

- Graphical file picker (select `.osr` + `.osu`/`.osz`)
- Auto-detects the correct beatmap from osu!lazer's file store via MD5
- Extracts audio directly from `.osz` files
- Scroll velocity (SV) support
- Hold note rendering
- Live HUD: score, combo, accuracy, and judgement counters
- MP4 export with audio sync via FFmpeg
- Real-time preview mode

## Requirements

- osu!lazer installed (for auto beatmap detection)
- FFmpeg

## Dependencies (build)

- GCC / Clang (C++17)
- CMake + Ninja
- SFML 3.x
- liblzma

On Arch Linux:

```bash
sudo pacman -S base-devel cmake ninja sfml ffmpeg xz
```

## Build

```bash
git clone https://github.com/itsjotaa/mania-replay-renderer
cd mania-replay-renderer
cmake -B build -G Ninja
ninja -C build
./build/mania-renderer
```

Windows builds are available in [Releases](../../releases).

## How it works

| Module | Description |
|---|---|
| `OsrParser` | Reads binary `.osr` format, decompresses LZMA replay data, extracts keypress frames |
| `OsuParser` | Parses `.osu` text format, extracts notes, timing points, and SVs |
| `OszExtractor` | Extracts `.osu` files and audio from `.osz` archives |
| `OsuFinder` | Searches osu!lazer's file store and matches beatmaps by MD5 |
| `ReplayProcessor` | Crosses keypress timestamps with note timings to assign judgements |
| `ScrollCalculator` | Computes per-frame note positions accounting for SV changes |
| `Renderer` | Draws columns, notes, hold bodies, key highlights, and HUD via SFML |
| `FFmpegPipe` | Streams raw RGBA frames to FFmpeg via stdin for H.264 encoding |

## Known limitations

- 4K only (7K, 9K planned)
- Accuracy display may differ slightly from in-game (~1-2%)

## Roadmap

- [x] Graphical file picker (v0.2)
- [x] .osz support + auto-detect beatmap (v0.3)
- [ ] Skin support
- [ ] Multiple keymodes (7K, 9K)
- [ ] Configurable output path 

## License

MIT
