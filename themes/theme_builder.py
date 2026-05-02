#!/usr/bin/env python3
"""
theme_builder.py — Build a .theme binary file from a local theme folder.

Usage:
    python themes/theme_builder.py <theme_name> <output.theme>

The .theme binary format:
    Version 1 header (8 bytes):
        magic[4]       = 'T','H','M','E'
        version[2]     = 1  (little-endian uint16)
        nb_colors[2]   = N  (little-endian uint16)

    Version 2 header (10 bytes):
        magic[4]       = 'T','H','M','E'
        version[2]     = 2  (little-endian uint16)
        nb_colors[2]   = N  (little-endian uint16)
        nb_icons[2]    = M  (little-endian uint16)

    Color entries (N × variable length):
        name_len[1]    = length of the key string (uint8)
        name[name_len] = key name (ASCII, no null terminator)
        rgb565[2]      = RGB565 color value (little-endian uint16)

    Icon entries (M × variable length, v2 only):
        name_len[1]    = length of the firmware path key (uint8)
        name[name_len] = firmware path (e.g. "apps/exam_icon.png")
        width[2]       = icon width in pixels (little-endian uint16)
        height[2]      = icon height in pixels (little-endian uint16)
        data_len[4]    = length of compressed data (little-endian uint32)
        data[data_len] = LZ4 block-compressed RGB565 pixel data

Fallback: keys missing from the requested theme fall back to beta_dark,
then to the hard-coded defaults in themes_manager.py (same as the compiler).
"""

import sys
import os
import json
import struct
import argparse

THEMES_LOCAL_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), "themes", "local")
ICONS_JSON       = os.path.join(os.path.dirname(os.path.realpath(__file__)), "icons.json")
FALLBACK_THEME   = "beta_dark"

MAGIC   = b"THME"
VERSION = 2


def rgb24_to_rgb565(hex_str: str) -> int:
    """Convert a 6-hex-digit RGB24 string to an RGB565 integer."""
    r = int(hex_str[0:2], 16)
    g = int(hex_str[2:4], 16)
    b = int(hex_str[4:6], 16)
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def load_theme_json(theme_name: str) -> dict:
    path = os.path.join(THEMES_LOCAL_DIR, theme_name + ".json")
    if not os.path.isfile(path):
        print(f"ERROR: Theme '{theme_name}' not found at {path}", file=sys.stderr)
        sys.exit(1)
    with open(path, "r") as f:
        return json.load(f)


def theme_to_flat_dict(data: dict) -> dict:
    """Flatten the nested JSON color structure into the same key format as themes_manager.py."""
    code_special_keys = {"parenthese_1", "parenthese_2", "parenthese_3", "invalid_parenthese"}
    result = {}
    for key, value in data["colors"].items():
        if isinstance(value, str):
            result[key] = value
        else:
            for sub_key, sub_value in value.items():
                if key == "Code" and sub_key in code_special_keys:
                    result[sub_key] = sub_value
                else:
                    result[key + sub_key] = sub_value
    return result


def build_theme_binary(theme_name: str) -> bytes:
    """Build the .theme binary for the given theme name."""
    # Hard-coded defaults (same as themes_manager.py)
    defaults = {
        "YellowDark": "ffb734",
        "YellowLight": "ffcc7b",
        "PurpleBright": "656975",
        "PurpleDark": "414147",
        "GrayWhite": "f5f5f5",
        "GrayBright": "ececec",
        "GrayMiddle": "d9d9d9",
        "GrayDark": "a7a7a7",
        "GrayVeryDark": "8c8c8c",
        "Select": "d4d7e0",
        "SelectDark": "b0b8d8",
        "WallScreen": "f7f9fa",
        "WallScreenDark": "e0e6ed",
        "SubTab": "b8bbc5",
        "LowBattery": "f30211",
        "Red": "ff000c",
        "RedLight": "fe6363",
        "Magenta": "ff0588",
        "Turquoise": "60c1ec",
        "Pink": "ffabb6",
        "Blue": "5075f2",
        "BlueLight": "718fee",
        "Orange": "fe871f",
        "Green": "50c102",
        "GreenLight": "52db8f",
        "Brown": "8d7350",
        "Purple": "6e2d79",
        "BlueishGrey": "919ea4",
        "Cyan": "00ffff",
        "parenthese_1": "ffd400",
        "parenthese_2": "50c102",
        "parenthese_3": "5075f2",
        "invalid_parenthese": "ff000c",
    }

    # Apply fallback theme (beta_dark)
    if theme_name != FALLBACK_THEME:
        fallback_data = load_theme_json(FALLBACK_THEME)
        defaults.update(theme_to_flat_dict(fallback_data))

    # Apply requested theme
    theme_data = load_theme_json(theme_name)
    merged = dict(defaults)
    merged.update(theme_to_flat_dict(theme_data))

    # Encode color entries
    color_entries = []
    for key, hex_val in merged.items():
        key_bytes = key.encode("ascii")
        if len(key_bytes) > 255:
            print(f"WARNING: key '{key}' too long, skipping.", file=sys.stderr)
            continue
        rgb565 = rgb24_to_rgb565(hex_val)
        entry = struct.pack("B", len(key_bytes)) + key_bytes + struct.pack("<H", rgb565)
        color_entries.append(entry)

    # Encode icon entries (if icons exist in the theme folder and dependencies are available)
    icon_entries = encode_theme_icons(theme_name)

    nb_colors = len(color_entries)
    nb_icons  = len(icon_entries)

    if nb_icons > 0:
        header = MAGIC + struct.pack("<HHH", VERSION, nb_colors, nb_icons)
    else:
        # Emit v1 if no icons (smaller, backwards-compatible)
        header = MAGIC + struct.pack("<HH", 1, nb_colors)

    return header + b"".join(color_entries) + b"".join(icon_entries)


def png_to_lz4_rgb565(png_path: str):
    """Convert a PNG file to LZ4 block-compressed RGB565 data (same as inliner.c).
    Returns (width, height, compressed_bytes) or None if dependencies missing."""
    try:
        from PIL import Image as PILImage
        import lz4.block
    except ImportError:
        return None

    img = PILImage.open(png_path).convert("RGBA")
    w, h = img.size
    pixels = img.load()

    raw_pixels = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            # Blend with white background (same as inliner.c)
            alpha = a / 255.0
            r = int(r * alpha + 255 * (1.0 - alpha))
            g = int(g * alpha + 255 * (1.0 - alpha))
            b = int(b * alpha + 255 * (1.0 - alpha))
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            raw_pixels.append(struct.pack("<H", rgb565))

    raw = b"".join(raw_pixels)
    compressed = lz4.block.compress(raw, store_size=False, mode="high_compression")
    return w, h, compressed


def encode_theme_icons(theme_name: str) -> list:
    """Build icon binary entries for all icons present in the theme's folder."""
    if not os.path.isfile(ICONS_JSON):
        return []

    with open(ICONS_JSON, "r") as f:
        icons_map = json.load(f)  # firmware_path → theme_relative_path

    theme_dir = os.path.join(THEMES_LOCAL_DIR, theme_name)
    if not os.path.isdir(theme_dir):
        return []

    entries = []
    deps_warned = False

    for firmware_path, theme_rel_path in icons_map.items():
        # Skip the bootloader computer image – it's not relevant for app theming
        if theme_rel_path == "bootloader/computer.png":
            continue

        icon_png = os.path.join(theme_dir, theme_rel_path)
        if not os.path.isfile(icon_png):
            continue  # icon not present in this theme

        result = png_to_lz4_rgb565(icon_png)
        if result is None:
            if not deps_warned:
                print("WARNING: Pillow or lz4 not installed – icons skipped. "
                      "Run: pip install Pillow lz4", file=sys.stderr)
                deps_warned = True
            continue

        w, h, compressed = result
        key_bytes = firmware_path.encode("ascii")
        if len(key_bytes) > 255:
            continue
        entry = (struct.pack("B", len(key_bytes)) + key_bytes
                 + struct.pack("<HHI", w, h, len(compressed))
                 + compressed)
        entries.append(entry)

    return entries


def main():
    parser = argparse.ArgumentParser(
        description="Build a .theme binary from a local theme.")
    parser.add_argument("theme", help="Theme name (e.g. beta_dark, omega_light)")
    parser.add_argument("output", help="Output .theme file path")
    parser.add_argument("-l", "--list", action="store_true",
                        help="List available local themes and exit")
    args = parser.parse_args()

    if args.list:
        print("Available local themes:")
        for f in sorted(os.listdir(THEMES_LOCAL_DIR)):
            if f.endswith(".json"):
                print(" ", f[:-5])
        sys.exit(0)

    data = build_theme_binary(args.theme)

    with open(args.output, "wb") as f:
        f.write(data)

    # Parse header to report counts
    version = struct.unpack_from("<H", data, 4)[0]
    nb_colors = struct.unpack_from("<H", data, 6)[0]
    nb_icons = struct.unpack_from("<H", data, 8)[0] if version == 2 else 0
    print(f"Written {len(data)} bytes → {args.output}  "
          f"({nb_colors} colors, {nb_icons} icons, format v{version})")


if __name__ == "__main__":
    main()
