# cgalaxy

<p align="center">
  <strong>A slowly rotating spiral galaxy for your terminal.</strong>
</p>

`cgalaxy` renders a colorful, procedural galaxy using C99 and ncurses. Every
run forms a new system with two to four spiral arms, a luminous core, nebula
dust, differential rotation, and a field of twinkling background stars.

It is lightweight, responsive to terminal resizing, and uses your existing
terminal background—including transparent backgrounds.

## Features

- Procedural spiral galaxies with a dense galactic core
- Counterclockwise differential rotation
- Five 256-color palettes
- Live controls for speed, density, palette, and arm count
- Unicode and ASCII rendering modes
- Automatic terminal resize handling
- No runtime dependencies beyond ncurses

## Installation

### macOS

Install Apple's command-line developer tools if you do not already have them:

```sh
xcode-select --install
```

Then build and install `cgalaxy`:

```sh
git clone https://github.com/gibsonmurray/cgalaxy.git
cd cgalaxy
make
make install PREFIX="$HOME/.local"
```

Make sure `$HOME/.local/bin` is on your `PATH`. For zsh:

```sh
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
exec zsh
```

### Ubuntu or Debian

```sh
sudo apt update
sudo apt install build-essential libncurses-dev
git clone https://github.com/gibsonmurray/cgalaxy.git
cd cgalaxy
make
sudo make install
```

### Other Unix-like systems

Install a C99 compiler, `make`, and the ncurses development package, then run:

```sh
make
make install PREFIX="$HOME/.local"
```

## Usage

Start the galaxy:

```sh
cgalaxy
```

Choose a palette or tune the animation at launch:

```sh
cgalaxy -p ultraviolet
cgalaxy -d 9 -s 5 -f 30
cgalaxy -a
```

### Command-line options

| Option | Description |
| --- | --- |
| `-f FPS` | Frames per second, from 5 to 60 (default: 24) |
| `-d NUM` | Star density, from 1 to 10 (default: 6) |
| `-s NUM` | Rotation speed, from 0 to 10 (default: 3) |
| `-p NAME` | Palette: `milky-way`, `andromeda`, `ultraviolet`, `ember`, or `ice` |
| `-a` | Use ASCII glyphs only |
| `-h` | Show help |
| `-v` | Show version |

### Live controls

| Key | Action |
| --- | --- |
| `q` or `Esc` | Quit |
| `Space` | Pause or resume |
| `r` | Form a new galaxy |
| `c` / `C` | Next / previous palette |
| `a` | Toggle ASCII glyphs |
| `d` / `D` | More / fewer stars |
| `+` / `-` | Faster / slower rotation |
| `[` / `]` | Fewer / more spiral arms |

## Build targets

```sh
make          # build
make check    # run command-line checks
make clean    # remove build output
make install  # install to PREFIX/bin
make uninstall
```

The default installation prefix is `/usr/local`. Set `PREFIX` to install
elsewhere:

```sh
make install PREFIX="$HOME/.local"
```

## How it works

The renderer generates thousands of particles distributed between a central
bulge and logarithmic spiral arms. Stars closer to the core orbit slightly
faster than stars near the edge, producing differential rotation. The system
is tilted and projected into terminal coordinates, then depth, brightness,
twinkle, and palette information determine the glyph and color drawn in each
cell.

## Uninstall

Use the same prefix that you chose during installation:

```sh
make uninstall PREFIX="$HOME/.local"
```

## Acknowledgments

Inspired by [realstrawhat/csakura](https://github.com/realstrawhat/csakura),
a lovely falling-sakura animation for the terminal.

## License

[MIT](LICENSE)
