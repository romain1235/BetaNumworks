#include "display.h"
#include "framebuffer.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <ion/display.h>
#include <SDL.h>
#include <string.h>

namespace Ion {
namespace Simulator {
namespace Display {

static SDL_Texture * sTexture = nullptr;
static int sTextureWidth = 0;
static int sTextureHeight = 0;

static void destroyTexture() {
  if (sTexture != nullptr) {
    SDL_DestroyTexture(sTexture);
    sTexture = nullptr;
    sTextureWidth = 0;
    sTextureHeight = 0;
  }
}

static void ensureTexture(SDL_Renderer * renderer, int width, int height) {
  if (sTexture != nullptr && sTextureWidth == width && sTextureHeight == height) {
    return;
  }
  destroyTexture();
  sTexture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_RGB565,
    SDL_TEXTUREACCESS_STREAMING,
    width,
    height
  );
  sTextureWidth = width;
  sTextureHeight = height;
}

/* Box filter: each destination pixel averages the source area it covers.
 * - Downscale: every source pixel still contributes (thin features stay visible).
 * - Upscale: source pixels stay sharp blocks; blending only happens on edges
 *   that fall between two source pixels (non-integer scales). */
static KDColor sampleBox(const KDColor * src, float x0, float y0, float x1, float y1) {
  constexpr int srcW = Ion::Display::Width;
  constexpr int srcH = Ion::Display::Height;

  x0 = std::max(0.f, x0);
  y0 = std::max(0.f, y0);
  x1 = std::min(static_cast<float>(srcW), x1);
  y1 = std::min(static_cast<float>(srcH), y1);
  if (x1 <= x0 || y1 <= y0) {
    return KDColorBlack;
  }

  int ix0 = static_cast<int>(x0);
  int iy0 = static_cast<int>(y0);
  int ix1 = std::min(srcW - 1, static_cast<int>(std::ceil(x1)) - 1);
  int iy1 = std::min(srcH - 1, static_cast<int>(std::ceil(y1)) - 1);

  float sumR = 0.f;
  float sumG = 0.f;
  float sumB = 0.f;
  float sumW = 0.f;
  for (int y = iy0; y <= iy1; y++) {
    float fy0 = std::max(y0, static_cast<float>(y));
    float fy1 = std::min(y1, static_cast<float>(y + 1));
    float wy = fy1 - fy0;
    for (int x = ix0; x <= ix1; x++) {
      float fx0 = std::max(x0, static_cast<float>(x));
      float fx1 = std::min(x1, static_cast<float>(x + 1));
      float w = (fx1 - fx0) * wy;
      KDColor c = src[y * srcW + x];
      sumR += w * static_cast<float>(c.red());
      sumG += w * static_cast<float>(c.green());
      sumB += w * static_cast<float>(c.blue());
      sumW += w;
    }
  }
  if (sumW <= 0.f) {
    return KDColorBlack;
  }
  return KDColor::RGB888(
    static_cast<uint8_t>(sumR / sumW + 0.5f),
    static_cast<uint8_t>(sumG / sumW + 0.5f),
    static_cast<uint8_t>(sumB / sumW + 0.5f)
  );
}

static void scaleFramebuffer(KDColor * dst, int dstPitchPixels, int dstW, int dstH) {
  constexpr int srcW = Ion::Display::Width;
  constexpr int srcH = Ion::Display::Height;
  const KDColor * src = Framebuffer::address();

  if (dstW == srcW && dstH == srcH) {
    for (int y = 0; y < dstH; y++) {
      memcpy(dst + y * dstPitchPixels, src + y * srcW, sizeof(KDColor) * srcW);
    }
    return;
  }

  for (int dy = 0; dy < dstH; dy++) {
    KDColor * row = dst + dy * dstPitchPixels;
    float y0 = static_cast<float>(dy) * static_cast<float>(srcH) / static_cast<float>(dstH);
    float y1 = static_cast<float>(dy + 1) * static_cast<float>(srcH) / static_cast<float>(dstH);
    for (int dx = 0; dx < dstW; dx++) {
      float x0 = static_cast<float>(dx) * static_cast<float>(srcW) / static_cast<float>(dstW);
      float x1 = static_cast<float>(dx + 1) * static_cast<float>(srcW) / static_cast<float>(dstW);
      row[dx] = sampleBox(src, x0, y0, x1, y1);
    }
  }
}

void init(SDL_Renderer * renderer) {
  Framebuffer::setActive(true);
  assert(sizeof(KDColor) == SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGB565));
  /* Texture size tracks the on-screen rect; filtering is done in software so
   * the final blit is 1:1 (same idea as a web canvas CSS-scaled from 320×240). */
  ensureTexture(renderer, Ion::Display::Width, Ion::Display::Height);
}

void shutdown() {
  destroyTexture();
}

void draw(SDL_Renderer * renderer, SDL_Rect * rect) {
  if (rect == nullptr || rect->w <= 0 || rect->h <= 0) {
    return;
  }

  ensureTexture(renderer, rect->w, rect->h);

  int pitch = 0;
  void * pixels = nullptr;
  SDL_LockTexture(sTexture, nullptr, &pixels, &pitch);
  assert(pitch >= static_cast<int>(sizeof(KDColor) * rect->w));
  scaleFramebuffer(
    static_cast<KDColor *>(pixels),
    pitch / static_cast<int>(sizeof(KDColor)),
    rect->w,
    rect->h
  );
  SDL_UnlockTexture(sTexture);

  SDL_RenderCopy(renderer, sTexture, nullptr, rect);
}

}
}
}
