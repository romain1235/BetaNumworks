import fitz
import unicodedata
import string

# Mapping simplifié pour les couleurs
COLOR_MAP = {
    (1, 0, 0): "%r%",
    (1, 0.5, 0): "%o%",
    (0, 1, 0): "%g%",
    (0, 1, 1): "%c%",
    (0, 0, 1): "%b%",
    (1, 0, 1): "%m%",
}

# Seuil pour considérer proche du noir
BLACK_THRESHOLD = 0.1

def is_black(r, g, b, threshold=BLACK_THRESHOLD):
    return r < threshold and g < threshold and b < threshold

def closest_color(rgb):
    r, g, b = rgb
    if is_black(r, g, b):
        return None
    best_dist = float('inf')
    best_code = None
    for (cr, cg, cb), code in COLOR_MAP.items():
        dist = (r-cr)**2 + (g-cg)**2 + (b-cb)**2
        if dist < best_dist:
            best_dist = dist
            best_code = code
    return best_code

def clean_text(text):
    # Normalisation unicode
    text = unicodedata.normalize("NFKC", text)

    # Remplacements utiles
    replacements = {
        "\u00a0": " ",  # espace insécable
        "Ï": "    ",
        "§": "",
        "©": "",
        "11 ": "",    # double espace vers simple
        "11#": "",
        "Þßà": "",
        "Â1Ä": "",
        "Ð": "",
    }
    for k, v in replacements.items():
        text = text.replace(k, v)

    # Échapper le % pour le parser URT
    text = text.replace("%", "(percent)")
    text = text.replace(",", ", ")
    text = text.replace(";", "; ")
    text = text.replace("_", "")

    # Garder uniquement caractères normaux
    allowed_extra = "{}[]<>#;=+-*/&|!\"'\\,.:"
    cleaned = ""
    for c in text:
        cat = unicodedata.category(c)
        if (
            c in "\n\t" or
            c in string.printable or
            cat.startswith("L") or
            cat.startswith("N") or
            cat.startswith("P") or
            c in allowed_extra
        ):
            cleaned += c
    return cleaned

def pdf_to_urt(pdf_path, urt_path):
    doc = fitz.open(pdf_path)

    if doc.is_encrypted:
        doc.authenticate("")

    with open(urt_path, "w", encoding="utf-8") as out:
        for page in doc:
            blocks = page.get_text("dict")["blocks"]
            lines_data = []

            for b in blocks:
                if "lines" not in b:
                    continue
                for line in b["lines"]:
                    spans = []
                    for span in line["spans"]:
                        # Récupère couleur
                        color_int = span["color"]
                        r = ((color_int >> 16) & 255) / 255
                        g = ((color_int >> 8) & 255) / 255
                        b_ = (color_int & 255) / 255
                        color_code = closest_color((r, g, b_))

                        spans.append({
                            "x": span["bbox"][0],
                            "text": clean_text(span["text"]),
                            "color": color_code
                        })
                    spans.sort(key=lambda s: s["x"])
                    lines_data.append({
                        "y": line["bbox"][1],
                        "spans": spans
                    })

            # Trier lignes verticalement
            lines_data.sort(key=lambda l: l["y"])
            for line in lines_data:
                current_color = None
                for span in line["spans"]:
                    color = span["color"]
                    if color != current_color:
                        if current_color:
                            out.write(f"%\\{current_color[1:-1]}%")
                        if color:
                            out.write(color)
                        current_color = color
                    out.write(span["text"])
                if current_color:
                    out.write(f"%\\{current_color[1:-1]}%")  # fermer couleur à la fin de la ligne
                out.write("\n")
            out.write("\n")
    doc.close()
    print("Conversion terminée !")

# Exemple d'utilisation
pdf_to_urt("cpp.pdf", "cpp.urt")