# Game of Life — fullscreen double-buffered 320x240, 1 bit/pixel
# Usage: run inside Upsilon simulator or device MicroPython REPL

from kandinsky import framebuffer, set_fullscreen
import time, random

WIDTH = 320
HEIGHT = 240

# Create two 1bpp palette framebuffers (2 colors)
fb_a = framebuffer(WIDTH, HEIGHT, 1, True)
fb_b = framebuffer(WIDTH, HEIGHT, 1, True)

# Ensure palette: 0 -> black, 1 -> white
fb_a.change_color(0, (0,0,0))
fb_a.change_color(1, (255,255,255))
fb_b.change_color(0, (0,0,0))
fb_b.change_color(1, (255,255,255))

# Random initial state
prob = 0.15
for y in range(HEIGHT):
    for x in range(WIDTH):
        fb_a.set_pixel(x, y, 1 if random.random() < prob else 0)

set_fullscreen(True)

try:
    src = fb_a
    dst = fb_b
    while True:
        # Compute next generation (toroidal wrap)
        for y in range(HEIGHT):
            y_m = (y - 1) % HEIGHT
            y_p = (y + 1) % HEIGHT
            for x in range(WIDTH):
                x_m = (x - 1) % WIDTH
                x_p = (x + 1) % WIDTH
                s = 0
                s += int(src.get_raw_index(x_m, y_m))
                s += int(src.get_raw_index(x,   y_m))
                s += int(src.get_raw_index(x_p, y_m))
                s += int(src.get_raw_index(x_m, y  ))
                s += int(src.get_raw_index(x_p, y  ))
                s += int(src.get_raw_index(x_m, y_p))
                s += int(src.get_raw_index(x,   y_p))
                s += int(src.get_raw_index(x_p, y_p))
                cur = int(src.get_raw_index(x, y))
                if s == 3 or (s == 2 and cur == 1):
                    dst.set_pixel(x, y, 1)
                else:
                    dst.set_pixel(x, y, 0)
        # Draw result and swap
        dst.draw(0, 0, 1)
        src, dst = dst, src
        time.sleep(0.05)
except KeyboardInterrupt:
    # restore normal display
    set_fullscreen(False)
    print('Game of Life stopped')
