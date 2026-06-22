import os
import numpy as np
from PIL import Image

input_folder = "../epsilon_light/probability"
output_folder = "probability"
os.makedirs(output_folder, exist_ok=True)

default_old_start = np.array([255, 255, 255], dtype=np.float32)
default_old_end   = np.array([255, 184, 25], dtype=np.float32)
default_new_start = np.array([0, 0, 0], dtype=np.float32)
default_new_end   = np.array([255, 212, 0], dtype=np.float32)

focused_old_start = np.array([212, 215, 224], dtype=np.float32)
focused_old_end   = np.array([251, 177, 0], dtype=np.float32)
focused_new_start = np.array([51, 51, 51], dtype=np.float32)
focused_new_end   = np.array([255, 212, 0], dtype=np.float32)

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

    if filename.startswith("focused"):
      old_start, old_end = focused_old_start, focused_old_end
      new_start, new_end = focused_new_start, focused_new_end
    else:
      old_start, old_end = default_old_start, default_old_end
      new_start, new_end = default_new_start, default_new_end

    old_vec = old_end - old_start
    old_len2 = np.dot(old_vec, old_vec)

    flat = arr.reshape(-1, 3)
    t = np.array([compute_t(p, old_start, old_vec, old_len2) for p in flat])
    t = t.reshape(h, w, 1)

    new_arr = new_start + t * (new_end - new_start)
    new_arr = np.clip(new_arr, 0, 255).astype(np.uint8)

    Image.fromarray(new_arr).save(os.path.join(output_folder, filename))
    print(f"{filename} OK")

print("Done"