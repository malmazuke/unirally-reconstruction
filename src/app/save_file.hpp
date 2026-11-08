#pragma once

#include "front_end.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>

namespace unirally::app {

// Where a platform keeps a user's application data (APP-PARITY, R-0110).
enum class SavePlatform : std::uint8_t { macos, windows, other_unix };
SavePlatform this_platform();

// The environment variables the default save location is read from; null when
// unset.
struct SaveLocationEnvironment {
  const char *home{}, *xdg_data_home{}, *app_data{};
};

// The save file the app keeps when it is not given `--save-file`: macOS
// `~/Library/Application Support/Unirally/unirally.sav`, Windows
// `%APPDATA%\Unirally\unirally.sav`, Linux and other Unix systems
// `$XDG_DATA_HOME/unirally/unirally.sav` (`~/.local/share` when it is unset or
// not absolute). Empty when the environment gives no home.
std::filesystem::path
default_save_file(SavePlatform platform,
                  const SaveLocationEnvironment &environment);
// The same for this process's platform and environment.
std::filesystem::path default_save_file();

// A save file that exists but is not a cartridge RAM image: it is refused, and
// kept as it is, rather than overwritten.
struct DamagedSaveFile : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// The cartridge RAM kept on disk between runs (SAVE-FILES): 8192 bytes, the
// original's image, so an emulator's save of this ROM loads too.
class SaveFile {
public:
  // Throws DamagedSaveFile for a file of another size, std::runtime_error when
  // the file cannot be read.
  explicit SaveFile(std::filesystem::path path);
  // The image found at start, if the file exists.
  const std::optional<CartridgeImage> &loaded() const { return loaded_; }
  // Writes the image when it differs from the last one written or loaded; a
  // temporary file and a rename, so a crash leaves the previous save whole.
  // Throws when the folder or the file cannot be written.
  void store(const CartridgeImage &image);

private:
  std::filesystem::path path_;
  std::optional<CartridgeImage> loaded_, written_;
};

} // namespace unirally::app
