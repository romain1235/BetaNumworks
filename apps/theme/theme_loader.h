#ifndef APPS_THEME_LOADER_H
#define APPS_THEME_LOADER_H

#include <stdint.h>
#include <stddef.h>

/**
 * ThemeLoader reads a .theme binary from external flash and applies it to the
 * Palette at runtime.  It also handles persistence via Ion::Storage.
 *
 * Binary format:
 *   magic[4]       = 'T','H','M','E'
 *   version[2]     = 1  (little-endian uint16)
 *   nb_colors[2]   = N  (little-endian uint16)
 *   For each color entry (variable length):
 *     name_len[1]  = length of key name (uint8)
 *     name[...]    = key name ASCII string
 *     rgb565[2]    = RGB565 color (little-endian uint16)
 */
class ThemeLoader {
public:
  static constexpr const char * k_themeExtension = "thpref";
  static constexpr const char * k_themeBaseName  = "active";
  static constexpr const char * k_themeFileExt   = ".theme";
  static constexpr size_t       k_maxThemeNameLength = 40;

  /** Apply the theme stored in Ion::Storage (if any). Called at boot. */
  static void applyStoredTheme();

  /** Load a named .theme file from external flash and apply it. Returns true on success. */
  static bool loadFromFlash(const char * themeFileName);

  /** Reset to the compiled-in palette defaults. */
  static void resetToDefault();

  /** Persist the active theme name to Ion::Storage. */
  static void persistThemeName(const char * themeFileName);

  /** Read the persisted theme name into buf (max k_maxThemeNameLength bytes). Returns false if none. */
  static bool readPersistedThemeName(char * buf, size_t bufSize);

  /** Number of .theme files available in external flash. */
  static int numberOfThemeFiles();

  /** Get the file name of the nth .theme file in external flash. */
  static bool themeFileAtIndex(int index, char * nameBuf, size_t bufSize);

  /** True when the active app should be reloaded to refresh cached themed views. */
  static bool needsAppReload();

  /** Clear the pending active-app reload request. */
  static void acknowledgeAppReload();

private:
  static constexpr uint8_t  k_magic[4]  = {'T', 'H', 'M', 'E'};
  static constexpr uint16_t k_version   = 1;

  static bool applyFromData(const uint8_t * data, size_t dataLength);
  static bool s_needsAppReload;
};

#endif
