# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- `.clang-format` and `.editorconfig` for code style consistency
- `CONTRIBUTING.md` with development guidelines
- `CHANGELOG.md` for tracking changes

### Changed
- Removed debug print statements from Renderer
- Removed commented-out code
- Fixed Spanish/English mix in comments

### Fixed
- Typos in comments (e.g., "health bard" → "health bar")
- Comment typos in OsrParser

## [v0.5] - 2026-05-04

### Added
- Lighting effects for key presses
- Skin support (in progress)
- Hit burst animations
- Real-time preview mode improvements

### Changed
- Improved rendering performance
- Better HUD layout

## [v0.4] - 2026-04-XX

### Added
- Scroll velocity (SV) support
- Hold note rendering
- Live HUD with score, combo, accuracy

### Fixed
- Timing point parsing
- Note position calculations

## [v0.3] - 2026-03-XX

### Added
- MP4 export with audio sync via FFmpeg
- Graphical file picker (UI)
- Auto-detect beatmap from osu!lazer file store

### Changed
- Replaced manual file selection with native dialogs

## [v0.2] - 2026-02-XX

### Added
- Basic 4K replay rendering
- `.osr` and `.osu` parsing
- Note judgement logic (320, 300, 200, 100, 50, MISS)
- SFML-based rendering

### Initial Features
- Column rendering
- Key press visualization
- Basic HUD display

---

[v0.5]: https://github.com/jetchhh/mania-replay-renderer/releases/tag/v0.5
[v0.4]: https://github.com/jetchhh/mania-replay-renderer/releases/tag/v0.4
[v0.3]: https://github.com/jetchhh/mania-replay-renderer/releases/tag/v0.3
[v0.2]: https://github.com/jetchhh/mania-replay-renderer/releases/tag/v0.2
