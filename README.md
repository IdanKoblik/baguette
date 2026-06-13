<div align="center">
  <h1>baguette</h1>

  <img src="assets/baguette.png" alt="baguette" width="400">

  <p>A minimal Wayland HUD / status bar written in C17.</p>
</div>

---

**baguette** is a lightweight overlay bar for `wlroots`-based Wayland
compositors. It draws a left / center / right status bar using
[Cairo](https://www.cairographics.org/) on a
[layer-shell](https://wayland.app/protocols/wlr-layer-shell-unstable-v1)
surface, and stays deliberately dumb: it does not collect any data itself.

Instead, baguette reads **frames on stdin** — one line per frame — and renders
them. You bring the content by piping any program into it. A ready-made
content generator,
[**za'atar**](https://github.com/IdanKoblik/zaatar), produces CPU/GPU temps,
memory, audio, uptime, date and workspace info out of the box (see
[Example](#example-zaatar)).

## Contents

- [Download](#download)
- [Build from source](#build-from-source)
- [Usage](#usage)
  - [Layout styles](#layout-styles)
  - [Input format](#input-format)
  - [Styling](#styling)
- [Example: za'atar](#example-zaatar)

## Download

Prebuilt `baguette` binaries are published on the
[Releases](https://github.com/IdanKoblik/baguette/releases) page.

```sh
# Grab the latest release binary
curl -L -o baguette https://github.com/IdanKoblik/baguette/releases/latest/download/baguette
chmod +x baguette
./baguette
```

The binary is dynamically linked against `wayland-client`, `cairo`,
`libconfig` and `libm`, so make sure those runtime libraries are installed.
If you'd rather build it yourself, see below.

### Arch Linux (PKGBUILD)

Arch users can build and install a proper package with the bundled
[`PKGBUILD`](PKGBUILD), which derives the version from git, runs the test
suite and pulls in the runtime dependencies automatically:

```sh
git clone https://github.com/IdanKoblik/baguette.git
cd baguette
makepkg -si
```

`makepkg -si` builds the package and installs it (and its dependencies) with
`pacman`.

## Build from source

baguette is plain C17 with a `Makefile` — no build system to learn.

**Dependencies:**

- A C compiler (`cc` / `gcc` / `clang`)
- `make`
- `pkg-config`
- `wayland-client` and `cairo` development packages
- `libconfig` development package
- `wayland-scanner` (generates the protocol code)

On Arch:

```sh
sudo pacman -S base-devel wayland cairo libconfig
```

**Build:**

```sh
make          # build the ./baguette binary
make run      # build and run
make clean    # remove build artifacts and generated protocol code
```

Optional developer targets:

```sh
make test      # build and run the unit test suite
make coverage  # run tests under coverage, emit an HTML report (needs lcov)
make compdb    # generate compile_commands.json for clangd (needs bear)
make logs      # export baguette's journald logs to baguette.log
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for code conventions and how to add
tests.

## Usage

Run baguette and feed it frames on stdin. The simplest possible test:

```sh
# A static one-frame example: left / center / right
printf 'hello\tcenter\tworld\n' | ./baguette
```

baguette keeps the bar up for as long as stdin is open, redrawing only when
the incoming frame actually changes. Send `SIGINT`/`SIGTERM` (or close the
pipe) to exit.

### Layout styles

| Flag           | Behaviour                                            |
| -------------- | ---------------------------------------------------- |
| `--separated`  | Separate rounded pills per section *(default)*       |
| `--full`       | One full-width background bar                        |

```sh
./baguette --full
```

### Input format

Each **line of stdin is one frame**. A frame is split into up to three
sections by tab characters:

```
LEFT  \t  CENTER  \t  RIGHT
```

Each section is plain text with optional inline colour tags:

| Tag           | Meaning                                          |
| ------------- | ------------------------------------------------ |
| `%{#rrggbb}`  | colour the text that follows                     |
| `%{-}`        | reset to the default colour                      |
| `%%`          | a literal `%`                                    |

A colour tag applies from where it appears until the next tag or the end of
the section, so one section can carry several differently-coloured runs.
Anything that isn't a recognised tag is kept literally — input is never
silently dropped.

```sh
# Red "CPU 72°C" on the right, default-coloured workspace on the left
printf ' 1 2 3\t%%{#ff5555}CPU 72°C%%{-}\n' | ./baguette
```

### Styling

Global look (font, colours, sizes) is read from a
[libconfig](https://hyperrealm.github.io/libconfig/) file at:

```
~/.config/baguette/style.cfg
```

You don't have to create it yourself: the **first time** baguette starts and
that file is missing, it writes a default `style.cfg` to that path. Run
baguette once, then open the file and tweak it to taste. (You can also copy
[`assets/style.cfg`](assets/style.cfg) there as a starting point.)

Available keys:

```ini
font = "JetbrainsMono Nerd Font";   # font family (default: "monospace")
font_size = 14.0;                   # point size (default: 14.0)

height = 32.0;                      # bar height in px (default: 32.0)
background = "#101010";             # "#RRGGBB" hex colour (default: "#000000")

hud_padding = 8.0;                  # inner padding around content (default: 8.0)
radius = 12.0;                      # corner radius of the pills (default: 5.0)
vmargin = 4.0;                      # vertical margin from the screen edge (default: 4.0)
pad_x = 6.0;                        # horizontal padding per section (default: 8.0)
```

Numbers may be written as integers or floats interchangeably (`radius = 12;`
and `radius = 12.0;` both work). After editing, restart baguette to pick up the
changes.

## Example: za'atar

[**za'atar**](https://github.com/IdanKoblik/zaatar) is a small Python content
generator built specifically for baguette. It emits exactly the
`left \t center \t right` wire format on stdout, so you just pipe it in:

```sh
# Clone the generator
git clone https://github.com/IdanKoblik/zaatar.git
cd zaatar

# Pipe its output straight into baguette
python3 main.py | baguette --full
```

za'atar fills the bar with:

- **left** — current workspaces
- **center** — date / time
- **right** — CPU & GPU temps, memory, PulseAudio volume, uptime, Tailscale
  status

To launch it automatically, add the pipeline to your compositor's startup
(e.g. in your Hyprland/Sway config):

```sh
exec python3 /path/to/zaatar/main.py | /path/to/baguette --full
```

Anything that can print `left \t center \t right` lines works just as well —
za'atar is just a convenient default.
