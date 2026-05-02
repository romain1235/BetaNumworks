#include "theme_loader.h"
#include <apps/external/archive.h>
#include <escher/palette.h>
#include <escher/image_store_override.h>
#include <apps/apps_container.h>
#include <ion/storage.h>
#include <string.h>

bool ThemeLoader::s_needsAppReload = false;

// ─── helpers ────────────────────────────────────────────────────────────────

static uint16_t readLE16(const uint8_t * p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static bool stringEndsWith(const char * str, const char * suffix) {
  size_t sl = strlen(str), xl = strlen(suffix);
  if (xl > sl) return false;
  return strcmp(str + sl - xl, suffix) == 0;
}

static uint32_t readLE32(const uint8_t * p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// ─── public API ─────────────────────────────────────────────────────────────

bool ThemeLoader::applyFromData(const uint8_t * data, size_t dataLength) {
  if (dataLength < 8) return false;
  if (data[0] != k_magic[0] || data[1] != k_magic[1] ||
      data[2] != k_magic[2] || data[3] != k_magic[3]) return false;

  uint16_t version   = readLE16(data + 4);
  uint16_t nbColors  = readLE16(data + 6);
  if (version != 1 && version != 2) return false;

  uint16_t nbIcons = 0;
  const uint8_t * p;
  if (version == 2) {
    if (dataLength < 10) return false;
    nbIcons = readLE16(data + 8);
    p = data + 10;
  } else {
    p = data + 8;
  }
  const uint8_t * end = data + dataLength;

  // Parse color entries
  for (uint16_t i = 0; i < nbColors; i++) {
    if (p >= end) break;
    uint8_t nameLen = *p++;
    if (p + nameLen + 2 > end) break;

    char key[64];
    if (nameLen >= sizeof(key)) { p += nameLen + 2; continue; }
    memcpy(key, p, nameLen);
    key[nameLen] = '\0';
    p += nameLen;

    uint16_t rgb565 = readLE16(p);
    p += 2;

    Palette::overrideColor(key, KDColor::RGB16(rgb565));
  }
  Palette::rebuildArrayColors();

  // Parse icon entries (v2 only)
  if (version == 2 && nbIcons > 0) {
    ImageStore::resetIconPool();
    for (uint16_t i = 0; i < nbIcons; i++) {
      if (p + 1 > end) break;
      uint8_t nameLen = *p++;
      if (p + nameLen + 8 > end) break;  // name + width(2) + height(2) + datalen(4)

      char key[64];
      if (nameLen >= sizeof(key)) {
        // Skip: name + width + height + datalen + data
        p += nameLen;
        if (p + 8 > end) break;
        uint32_t dataLen = readLE32(p + 4);
        p += 8 + dataLen;
        continue;
      }
      memcpy(key, p, nameLen);
      key[nameLen] = '\0';
      p += nameLen;

      if (p + 8 > end) break;
      uint16_t w       = readLE16(p);
      uint16_t h       = readLE16(p + 2);
      uint32_t dataLen = readLE32(p + 4);
      p += 8;

      if (p + dataLen > end) break;
      ImageStore::overrideIcon(key, p, w, h, dataLen);
      p += dataLen;
    }
  }

  s_needsAppReload = true;
  return true;
}

bool ThemeLoader::loadFromFlash(const char * themeFileName) {
  int idx = External::Archive::indexFromName(themeFileName);
  if (idx < 0) return false;

  External::Archive::File file;
  if (!External::Archive::fileAtIndex((size_t)idx, file)) return false;
  if (!file.readable) return false;

  return applyFromData(file.data, file.dataLength);
}

void ThemeLoader::resetToDefault() {
  Palette::resetToDefaults();
  ImageStore::resetIconPool();
  // Force UI refresh: reload title bar colors and redraw whole window
  AppsContainer::sharedAppsContainer()->reloadTitleBarView();
  AppsContainer::sharedAppsContainer()->redrawWindow(true);
  s_needsAppReload = true;
}

bool ThemeLoader::needsAppReload() {
  return s_needsAppReload;
}

void ThemeLoader::acknowledgeAppReload() {
  s_needsAppReload = false;
}

void ThemeLoader::applyStoredTheme() {
  char name[k_maxThemeNameLength];
  if (!readPersistedThemeName(name, sizeof(name))) return;
  if (name[0] == '\0') return;  // default theme
  loadFromFlash(name);
}

void ThemeLoader::persistThemeName(const char * themeFileName) {
  Ion::Storage * s = Ion::Storage::sharedStorage();
  // Destroy any existing record
  Ion::Storage::Record existing = s->recordBaseNamedWithExtension(k_themeBaseName, k_themeExtension);
  if (!existing.isNull()) {
    s->destroyRecord(existing);
  }
  if (themeFileName && themeFileName[0] != '\0') {
    size_t len = strlen(themeFileName) + 1;
    s->createRecordWithExtension(k_themeBaseName, k_themeExtension, themeFileName, len);
  }
}

bool ThemeLoader::readPersistedThemeName(char * buf, size_t bufSize) {
  Ion::Storage * s = Ion::Storage::sharedStorage();
  Ion::Storage::Record rec = s->recordBaseNamedWithExtension(k_themeBaseName, k_themeExtension);
  if (rec.isNull()) return false;
  Ion::Storage::Record::Data d = rec.value();
  if (d.size == 0 || d.size > bufSize) return false;
  memcpy(buf, d.buffer, d.size);
  buf[d.size - 1] = '\0';  // ensure null termination
  return true;
}

int ThemeLoader::numberOfThemeFiles() {
  int count = 0;
  size_t nb = External::Archive::numberOfFiles();
  for (size_t i = 0; i < nb; i++) {
    External::Archive::File file;
    if (External::Archive::fileAtIndex(i, file) && file.readable
        && stringEndsWith(file.name, k_themeFileExt)) {
      count++;
    }
  }
  return count;
}

bool ThemeLoader::themeFileAtIndex(int index, char * nameBuf, size_t bufSize) {
  int count = 0;
  size_t nb = External::Archive::numberOfFiles();
  for (size_t i = 0; i < nb; i++) {
    External::Archive::File file;
    if (External::Archive::fileAtIndex(i, file) && file.readable
        && stringEndsWith(file.name, k_themeFileExt)) {
      if (count == index) {
        size_t len = strlen(file.name);
        if (len >= bufSize) return false;
        memcpy(nameBuf, file.name, len + 1);
        return true;
      }
      count++;
    }
  }
  return false;
}
