# Tests for kandinsky.framebuffer features
# Run this script inside the Upsilon simulator or on-device MicroPython REPL.

from kandinsky import framebuffer, set_fullscreen

# helper: compute nearest palette color by Euclidean distance
def nearest_color(target, palette):
    tr, tg, tb = target
    best = None
    bestd = None
    for c in palette:
        r,g,b = c
        d = (r-tr)**2 + (g-tg)**2 + (b-tb)**2
        if best is None or d < bestd:
            best = c
            bestd = d
    return best


def test_change_color_and_fill_index():
    fb = framebuffer(8, 8, 4, True)  # 4 bits -> 16-color palette
    fb.change_color(0, (0,0,0))
    fb.change_color(1, (255,0,0))
    fb.change_color(2, (0,255,0))
    fb.change_color(3, (0,0,255))
    # fill using an index should paint palette color
    fb.fill(1)
    # debug: show palette entry and raw index
    try:
        pal1 = fb.get_palette_entry(1)
    except Exception:
        pal1 = None
    raw = fb.get_raw_index(0,0)
    p = fb.get_pixel(0,0)
    print('palette[1]=', pal1, 'raw_index_at_0_0=', raw, 'rgb=', p)
    assert tuple(p) == (255,0,0), "fill with index did not set expected palette color"
    print('test_change_color_and_fill_index: OK')


def test_palette_closest_lookup_on_draw():
    fb = framebuffer(11, 11, 4, True)
    # define a simple palette
    palette = [ (0,0,0), (200,0,0), (0,200,0), (0,0,200) ]
    for i,c in enumerate(palette):
        fb.change_color(i, c)
    # draw a circle with a color not in the palette
    target = (250, 10, 10)  # should be closest to palette[1]
    fb.draw_circle(5,5,3, target)
    got = tuple(fb.get_pixel(5,5))
    expected = nearest_color(target, palette)
    assert got == expected, f"expected {expected}, got {got}"
    print('test_palette_closest_lookup_on_draw: OK')


def test_draw_and_fullscreen_no_crash():
    fb = framebuffer(10, 6)  # default non-palette mode
    # fill with a color
    fb.fill((10, 200, 30))
    # draw something with scaling and without
    fb.draw(0,0,1)
    set_fullscreen(True)
    fb.draw(0,0,1)  # must not crash when fullscreen is enabled
    set_fullscreen(False)
    print('test_draw_and_fullscreen_no_crash: OK')


if __name__ == '__main__':
    print('Running kandinsky.framebuffer tests...')
    test_change_color_and_fill_index()
    test_palette_closest_lookup_on_draw()
    test_draw_and_fullscreen_no_crash()
    print('All tests passed.')
