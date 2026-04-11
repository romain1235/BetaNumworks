import fitz
import unicodedata

def clean_text(text):
    text = unicodedata.normalize("NFKC", text)

    replacements = {
        "\u2019": "'",
        "\u201c": '"',
        "\u201d": '"',
        "\u2013": "-",
        "\u00a0": " ",

        # caractères foireux du PDF
        "Ï": "    ",   # indentation (4 espaces)
        "§": "",
        "©": "",
        "Þ": "",      # à ajuster si besoin
        "ß": "",
        "à": "",
        "¨": "",
        "¹": "",
        "11": "",    # double espace vers simple
    }

    for k, v in replacements.items():
        text = text.replace(k, v)

    return text

def pdf_to_txt(pdf_path, txt_path):
    doc = fitz.open(pdf_path)
    full_text = ""

    for page in doc:
        full_text += page.get_text()

    doc.close()

    full_text = clean_text(full_text)

    with open(txt_path, "w", encoding="utf-8") as f:
        f.write(full_text)

pdf_to_txt("c++2.pdf", "cpp.txt")