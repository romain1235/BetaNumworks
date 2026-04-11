import os
import numpy as np
from PIL import Image

input_folder = "../epsilon_light/probability"
output_folder = "probability"
os.makedirs(output_folder, exist_ok=True)

# Dégradés par défaut
default_old_start = np.array([255, 255, 255], dtype=np.float32)  # blanc
default_old_end   = np.array([255, 184, 25], dtype=np.float32)   # orange
default_new_start = np.array([0, 0, 0], dtype=np.float32)        # noir
default_new_end   = np.array([255, 212, 0], dtype=np.float32)    # jaune

# Dégradé pour les fichiers "focused"
focused_old_start = np.array([212, 215, 224], dtype=np.float32)  # #d4d7e0
focused_old_end   = np.array([251, 177, 0], dtype=np.float32)    # #fbb100
focused_new_start = np.array([17, 17, 17], dtype=np.float32)     # #111111
focused_new_end   = np.array([255, 212, 0], dtype=np.float32)    # #ffd400

def compute_t(pixel, old_start, old_vec, old_len2):
    v = pixel - old_start
    t = np.dot(v, old_vec) / old_len2
    return np.clip(t, 0, 1)

for filename in os.listdir(input_folder):
    if filename.lower().endswith((".png", ".jpg", ".jpeg")):
        path = os.path.join(input_folder, filename)
        img = Image.open(path).convert("RGB")
        
        arr = np.array(img, dtype=np.float32)
        h, w, _ = arr.shape

        # Choix du dégradé
        if filename.startswith("focused"):
            old_start, old_end = focused_old_start, focused_old_end
            new_start, new_end = focused_new_start, focused_new_end
        else:
            old_start, old_end = default_old_start, default_old_end
            new_start, new_end = default_new_start, default_new_end

        old_vec = old_end - old_start
        old_len2 = np.dot(old_vec, old_vec)

        # Calcul du t pour chaque pixel
        flat = arr.reshape(-1, 3)
        t = np.array([compute_t(p, old_start, old_vec, old_len2) for p in flat])
        t = t.reshape(h, w, 1)

        # interpolation vers nouveau dégradé
        new_arr = new_start + t * (new_end - new_start)
        new_arr = np.clip(new_arr, 0, 255).astype(np.uint8)

        Image.fromarray(new_arr).save(os.path.join(output_folder, filename))
        print(f"{filename} OK")

print("Terminé ✅")