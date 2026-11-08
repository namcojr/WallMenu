# wallmenu

Minimal wallpaper picker for Linux built with Qt6 Widgets and C++20.

## Build

```bash
cmake -S . -B build
cmake --build build
cmake --build build && cmake --install build --prefix ~/.local
```

## Install

Install for the current user into `~/.local/bin` (make sure it is on your `PATH`):

```bash
cmake --install build --prefix ~/.local
```

Or system-wide into `/usr/local/bin`:

```bash
sudo cmake --install build
```

Both also install a `wallmenu.desktop` entry into `share/applications` under the prefix,
so wallmenu appears in application launchers such as rofi.

## Run

```bash
wallmenu            # if installed
./build/wallmenu    # from the build directory
```

## Wallpaper backends

The backend is chosen from `XDG_CURRENT_DESKTOP`:

- **KDE Plasma**: `plasma-apply-wallpaperimage` (Plasma 5.24+ / 6), falling back to a
  Plasma script via `qdbus6` / `qdbus`
- **Sway**: `swaymsg output * bg <image> fill`
- **Anything else**: `feh --bg-fill` (X11 only)

## Keyboard shortcuts

- Up / Down: navigate image list
- Enter: set selected wallpaper
- r: refresh image directory immediately
- q: quit immediately

## Configuration

Config file path:

`~/.config/wallmenu.conf`

Example:

```ini
wallpaper_dir=/home/user/Pictures/wallpapers
recursive=true

list_font_name=DejaVu Sans
list_font_size=11

title_font_name=DejaVu Serif
title_font_size=12
title_font_style=bold
```

Supported keys:

- `wallpaper_dir`: directory containing images
- `recursive`: true/false
- `list_font_name`: font family for the left list
- `list_font_size`: font size for the left list (6-72)
- `title_font_name`: font family for the title above preview
- `title_font_size`: font size for the title (6-72)
- `title_font_style`: `normal`, `bold`, `italic`, or `bold_italic`

Title format above preview image:

`filename · size · widthxheight`
