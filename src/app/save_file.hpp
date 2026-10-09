#pragma once

#include "front_end.hpp"

#include <filesystem>
#include <optional>

namespace unirally::app {

// The cartridge RAM kept on disk between runs (SAVE-FILES): 8192 bytes, the
// original's image, so an emulator's save of this ROM loads too.
class SaveFile {
public:
  explicit SaveFile(std::filesystem::path path);
  // The image found at start, if the file exists.
  const std::optional<CartridgeImage> &loaded() const { return loaded_; }
  // Writes the image when it differs from the last one written or loaded; a
  // temporary file and a rename, so a crash leaves the previous save whole.
  void store(const CartridgeImage &image);

private:
  std::filesystem::path path_;
  std::optional<CartridgeImage> loaded_, written_;
};

} // namespace unirally::app
