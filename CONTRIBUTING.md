# Contributing to mania-replay-renderer

Thank you for your interest in contributing! This guide will help you get started.

## Development Setup

### Prerequisites

- C++17 compatible compiler (GCC or Clang)
- CMake 3.20 or higher
- Ninja build system
- SFML 3.x
- liblzma
- miniz
- nativefiledialog-extended

### Building from Source

```bash
git clone https://github.com/jetchhh/mania-replay-renderer.git
cd mania-replay-renderer
cmake -B build -G Ninja
ninja -C build
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake ninja sfml ffmpeg xz
```

## Code Style

We use `.clang-format` and `.editorconfig` to maintain consistent formatting. Please run:

```bash
clang-format -i src/**/*.{cpp,hpp}
```

before submitting a pull request.

### Guidelines

- Use English for all comments and documentation
- Follow existing naming conventions (snake_case for variables, PascalCase for classes)
- Add comments for non-obvious logic
- Keep functions focused and concise

## Pull Request Process

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run clang-format on modified files
5. Test your changes thoroughly
6. Commit with clear, descriptive messages
7. Push to your fork and submit a Pull Request

### Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add support for 7K beatmaps
fix: correct lighting blend mode for hold notes
docs: update README with new features
refactor: simplify note rendering logic
```

## Testing

Currently, the project does not have automated tests. Manual testing is done with the provided `test.osr` and `test.osu` files.

When adding features, please test:
- 4K replay rendering (preview and export)
- Different scroll speeds
- Various skin configurations
- Edge cases (very fast/slow songs, custom timing points)

## Reporting Issues

When reporting bugs, please include:
- OS and version
- Steps to reproduce
- Expected vs actual behavior
- Sample `.osr` and `.osu` files if applicable

## Feature Requests

Feature requests are welcome! Please check the roadmap in README.md first.

Current priorities:
- 7K/9K support
- Full skin support
- Configurable output path
- Automated testing

## Questions?

Feel free to open an issue for any questions about contributing.
