#include "save_file.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace unirally::app {

SaveFile::SaveFile(std::filesystem::path path) : path_(std::move(path)) {
  if (!std::filesystem::exists(path_))
    return;
  std::ifstream in(path_, std::ios::binary);
  const std::vector<char> bytes((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
  CartridgeImage image{};
  if (!in.eof() || bytes.size() != image.size())
    throw std::runtime_error("a save file is 8192 bytes: " + path_.string());
  std::copy(bytes.begin(), bytes.end(), image.begin());
  loaded_ = written_ = image;
}

void SaveFile::store(const CartridgeImage &image) {
  if (written_ == image)
    return;
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
