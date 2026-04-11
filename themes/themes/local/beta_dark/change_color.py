#!/usr/bin/env python3
from pathlib import Path
import re
import shutil
import argparse

import cairosvg


def normalize_hex(color: str) -> str:
    color = color.strip().lower()
    if color.startswith("#"):
        color = color[1:]
    if len(color) != 6 or any(c not in "0123456789abcdef" for c in color):
        raise ValueError(f"Couleur invalide: {color!r} (attendu: 6 hex, ex: ffd400)")
    return color


def replace_hex_color(text: str, start_color: str, end_color: str) -> str:
    """
    Remplace la couleur start_color par end_color dans un texte.
    Gère:
      - ffd400
      - #ffd400
      - FFD400
      - #FFD400
      - et aussi les versions 8 digits en conservant l'alpha, ex: ffd40080
    """
    start_color = normalize_hex(start_color)
    end_color = normalize_hex(end_color)

    # Match exact 6 hex chars, avec ou sans #, et alpha optionnel de 2 chars
    pattern = re.compile(
        rf"(?i)(?<![0-9a-f])(?P<hash>#?){re.escape(start_color)}(?P<alpha>[0-9a-f]{{2}})?(?![0-9a-f])"
    )

    def repl(match: re.Match) -> str:
        prefix = match.group("hash") or ""
        alpha = match.group("alpha") or ""
        return f"{prefix}{end_color}{alpha}"

    return pattern.sub(repl, text)


def process_svg(svg_path: Path, start_color: str, end_color: str) -> None:
    svg_text = svg_path.read_text(encoding="utf-8")
    new_svg_text = replace_hex_color(svg_text, start_color, end_color)

    # On écrit un SVG temporaire en mémoire puis on le rend en PNG
    out_png = svg_path.with_suffix(".png")

    cairosvg.svg2png(
        bytestring=new_svg_text.encode("utf-8"),
        write_to=str(out_png),
        output_width=55,
        output_height=56,
    )

    print(f"OK  {svg_path} -> {out_png}")


def process_json(json_path: Path, start_color: str, end_color: str) -> None:
    if not json_path.exists():
        print(f"ATTENTION: JSON introuvable: {json_path}")
        return

    original = json_path.read_text(encoding="utf-8")
    modified = replace_hex_color(original, start_color, end_color)

    backup_path = json_path.with_suffix(json_path.suffix + ".bak")
    shutil.copy2(json_path, backup_path)

    json_path.write_text(modified, encoding="utf-8")
    print(f"OK  {json_path} (backup: {backup_path})")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("start_color", help="Couleur de départ, ex: ffd400")
    parser.add_argument("end_color", help="Couleur d'arrivée, ex: 45ada8")
    parser.add_argument("--apps", default="apps", help="Dossier contenant les SVG (défaut: apps)")
    parser.add_argument(
        "--json",
        default="../beta_dark.json",
        help="Chemin du JSON à modifier (défaut: ../beta_dark.json)",
    )
    args = parser.parse_args()

    start_color = normalize_hex(args.start_color)
    end_color = normalize_hex(args.end_color)

    apps_dir = Path(args.apps)
    json_path = Path(args.json)

    if not apps_dir.exists():
        raise FileNotFoundError(f"Dossier introuvable: {apps_dir}")

    svg_files = sorted(apps_dir.rglob("*.svg"))
    if not svg_files:
        print(f"Aucun SVG trouvé dans {apps_dir}")
    else:
        for svg_path in svg_files:
            process_svg(svg_path, start_color, end_color)

    process_json(json_path, start_color, end_color)


if __name__ == "__main__":
    main()