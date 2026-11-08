#include "save_file.hpp"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace unirally::app {

SavePlatform this_platform() {
#if defined(__APPLE__)
  return SavePlatform::macos;
#elif defined(_WIN32)
  return SavePlatform::windows;
#else
  return SavePlatform::other_unix;
#endif
}

std::filesystem::path
default_save_file(SavePlatform platform,
                  const SaveLocationEnvironment &environment) {
  const auto set = [](const char *value) { return value && *value; };
  constexpr auto file = "unirally.sav";
  switch (platform) {
  case SavePlatform::macos:
    if (!set(environment.home))
      return {};
    return std::filesystem::path(environment.home) / "Library" /
           "Application Support" / "Unirally" / file;
  case SavePlatform::windows:
    if (!set(environment.app_data))
      return {};
    return std::filesystem::path(environment.app_data) / "Unirally" / file;
  case SavePlatform::other_unix:
    break;
  }
  // The XDG base directory specification ignores a relative XDG_DATA_HOME.
  if (set(environment.xdg_data_home) &&
      std::filesystem::path(environment.xdg_data_home).is_absolute())
    return std::filesystem::path(environment.xdg_data_home) / "unirally" / file;
  if (!set(environment.home))
    return {};
  return std::filesystem::path(environment.home) / ".local" / "share" /
         "unirally" / file;
}

std::filesystem::path default_save_file() {
  return default_save_file(this_platform(),
                           {std::getenv("HOME"), std::getenv("XDG_DATA_HOME"),
                            std::getenv("APPDATA")});
}

SaveFile::SaveFile(std::filesystem::path path) : path_(std::move(path)) {
  if (!std::filesystem::exists(path_))
    return;
  std::ifstream in(path_, std::ios::binary);
  if (!in)
    throw std::runtime_error("cannot read the save file: " + path_.string());
  const std::vector<char> bytes((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
  CartridgeImage image{};
  if (bytes.size() != image.size())
    throw std::runtime_error("a save file is 8192 bytes: " + path_.string());
  std::copy(bytes.begin(), bytes.end(), image.begin());
  loaded_ = written_ = image;
}

void SaveFile::store(const CartridgeImage &image) {
  if (written_ == image)
    return;
  // The default save's folder may not exist yet (`default_save_file`).
  if (path_.has_parent_path())
    std::filesystem::create_directories(path_.parent_path());
  auto temporary = path_;
  temporary += ".tmp";
  {
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(image.data()),
              static_cast<std::streamsize>(image.size()));
    if (!out)
      throw std::runtime_error("cannot write the save file: " +
                               temporary.string());
  }
  std::filesystem::rename(temporary, path_);
  written_ = image;
}

} // namespace unirally::app
