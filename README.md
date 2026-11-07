# wallmenu

Minimal wallpaper picker for Linux built with Qt6 Widgets and C++20.

## Build

```bash
cmake -S . -B build
cmake --build build
cmake --build build && cmake --install build --prefix ~/.local
```

## Run

```bash
./build/wallmenu
```

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
