# Framebuffer feature test script
# Run on device/simulator where kandinsky module is available

from kandinsky import *

def show_pixel(fb, x, y, note=''):
    try:
        c = fb.get_pixel(x, y)
    except Exception as e:
        print('get_pixel error:', e)
        return
    print(f"{note} pixel ({x},{y}) = {c}")


def test_rgb222():
    print('\n=== TEST rgb222 (default) ===')
    fb = framebuffer(20, 10)
    fb.fill((255,0,0))
    show_pixel(fb, 0, 0, 'after fill red')
    fb.set_pixel(1, 1, (0,255,0))
    show_pixel(fb, 1, 1, 'set green')
    fb.draw(0,0)
    fb.close()


def test_rgb332():
    print('\n=== TEST rgb332 (8 bits) ===')
    fb = framebuffer(20, 10, 8)
    fb.fill((0,0,255))
    show_pixel(fb, 0, 0, 'after fill blue')
    fb.set_pixel(2, 2, (255,255,0))
    show_pixel(fb, 2, 2, 'set yellow')
    fb.draw(0,0)
    fb.close()


def test_rgb565():
    print('\n=== TEST rgb565 (16 bits) ===')
    fb = framebuffer(16, 16, 16)
    fb.fill((128,128,0))
    show_pixel(fb, 0, 0, 'after fill olive')
    fb.draw_line(0,0,15,15,(255,255,255))
    fb.draw(0,0)
    fb.close()


def test_palette():
    print('\n=== TEST palette mode (4 bits) ===')
    fb = framebuffer(16, 16, 4, True)  # palette with 16 entries
    # palette is pre-filled with black; change some entries
    fb.change_color(0, (255,0,0))   # index 0 = red
    fb.change_color(1, (0,255,0))   # index 1 = green
    fb.change_color(2, (0,0,255))   # index 2 = blue
    fb.fill(0)
    show_pixel(fb, 0, 0, 'after fill index 0 (red)')
    fb.set_pixel(4,4, 1)  # set by index
    show_pixel(fb, 4, 4, 'set by index 1 (green)')
    # change an entry and verify pixels using that index will show new color after redraw
    fb.change_color(0, (10,20,30))
    # write index 0 at a pixel
    fb.set_pixel(5,5, 0)
    show_pixel(fb, 5,5, 'after change_color(0,...) and set_pixel index 0')
    # drawing primitives accept indices or rgb tuples
    fb.fill_rect(0,0,6,2, 2)  # fill rect with palette index 2 (blue)
    fb.draw_string('P', 0,0, (255,255,255), (0,0,0))
    fb.draw(0,0)
    fb.close()


def test_shapes_on_scaled_draw():
    print('\n=== TEST shapes + scaled draw ===')
    fb = framebuffer(24, 24)
    fb.fill((0,0,0))
    fb.draw_circle(12,12,8,(255,0,255))
    fb.fill_circle(12,12,4,(0,255,255))
    fb.draw_line(0,0,23,23,(255,255,255))
    fb.draw_string('T', 2, 2, (255,255,0))
    fb.draw(10,10,2)  # draw scaled at (10,10)
    show_pixel(fb, 12,12, 'center (should be cyan)')
    fb.close()


def main():
    test_rgb222()
    test_rgb332()
    test_rgb565()
    test_palette()
    test_shapes_on_scaled_draw()
    print('\nAll tests finished.')

if __name__ == '__main__':
    main()
