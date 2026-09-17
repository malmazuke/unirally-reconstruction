#include "content_pack.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace unirally {
namespace {
class Reader {
public:
  explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
  std::uint16_t u16() {
    require(2);
    const auto v = static_cast<std::uint16_t>(
        bytes_[at_] | (static_cast<unsigned>(bytes_[at_ + 1]) << 8U));
    at_ += 2;
    return v;
  }
  std::uint32_t u32() {
    const auto lo = u16();
    return static_cast<std::uint32_t>(lo) |
           (static_cast<std::uint32_t>(u16()) << 16U);
  }
  std::uint64_t u64() {
    const auto lo = u32();
    return static_cast<std::uint64_t>(lo) |
           (static_cast<std::uint64_t>(u32()) << 32U);
  }
  std::array<std::uint8_t, 32> digest() {
    std::array<std::uint8_t, 32> value{};
    for (auto &byte : value) {
      byte = u8();
    }
    return value;
  }
  std::uint8_t u8() {
    require(1);
    return bytes_[at_++];
  }
  std::string text() {
    const auto width = u16();
    require(width);
    std::string value(bytes_.begin() + static_cast<std::ptrdiff_t>(at_),
                      bytes_.begin() +
                          static_cast<std::ptrdiff_t>(at_ + width));
    at_ += width;
    if (value.empty())
      throw std::invalid_argument("Classic pack contains an empty identity");
    return value;
  }
  void skip(std::size_t width) {
    require(width);
    at_ += width;
  }
  std::size_t offset() const { return at_; }

private:
  void require(std::size_t width) const {
    if (width > bytes_.size() - at_)
      throw std::invalid_argument("Classic pack is truncated");
  }
  std::span<const std::uint8_t> bytes_;
  std::size_t at_{};
};

struct RequiredEntry {
  std::string_view id;
  std::size_t size;
  std::string_view sha256;
};
const std::array<RequiredEntry, 25> required{
    {{"physics.track.dragster.data", 33815,
      "8f5cef67dc57977af8ff614b8178514a26e9fe0059e02ed27aad5978185580d4"},
     {"physics.rider.collision-poses", 32768,
      "9d1754d38c20cb2900239557550211ab6fc23d9b0237e17b78fb29f3bf272c32"},
     {"physics.rider.collision-templates", 17249,
      "2f03a8cb985899436603ef36b233b28f7b4213e4ba106fca328a6b02cdb081c7"},
     {"physics.track.progress-transitions", 80,
      "8a90513f349bc7836513d3495a9bbb90563b9eb7a716fd4aff5b9a4c74fc83df"},
     {"physics.track.dragster.tile-columns", 640,
      "bb95427aa2a307c9874b6140b145dce5a4e279112d57a5efcae77eb444951a72"},
     {"physics.track.dragster.tile-flags", 20,
      "590e52f2640bb1b70f602622aa07285ed51d7c4b84707b4d77c5cc33fc230846"},
     {"physics.speed.masks", 9,
      "0ca19a78da56137e0926c4ba602d8041648a422b9cf5d0a7c3de4a30998cf58b"},
     {"physics.speed.decrements", 18,
      "c1fab1d9aa1e691d34c1a78e8658cfd52b8334cc5bdec8efb23bedc672988068"},
     {"physics.rider.pose-slopes", 128,
      "f6b1ea6a34c78336ca25449c8ddbd23e2417ef829ec09765e695f957cd714584"},
     {"physics.rider.displacement-table", 512,
      "27894923de2aaeb58ca24dedbddadcf0d4d154fbc61ea484e7c248d066e24e1b"},
     {"physics.rider.idle-pose-table", 64,
      "05d2af9f8c0d1d8d8dd1915086f4f4c58456510f3daf357dbd7fdd66e4a8031e"},
     {"physics.reward.rotation-value", 2,
      "8509b81230019d2ad970d970f791dfbdc8caf54f5c594fcd327cef9feed206c1"},
     {"physics.reward.rotation-class", 1,
      "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"},
     {"presentation.track.dragster.bg1-tiles.v1", 2560,
      "5a45c158da5565b8f05872a2f23a9e5dcaa59a5a19d461700b624df67a2eff38"},
     {"presentation.track.dragster.bg2-tiles.v1", 992,
      "d50aaa4efde3d4b5eec805a69470faa368596087f5588d7d8940c95226bcb8a2"},
     {"presentation.track.dragster.bg2-map.v1", 8192,
      "574a44e71c96b210f5613f9b42e2de80e69e1aa64aaf59b8a3b3952f5f30d6b5"},
     {"presentation.classic.palette.v1", 352,
      "d98f7dfd1f0cd056aca52f2317a80609854738cc7b7e1c1e10f69ccdfea150ac"},
     {"presentation.classic.font.v1", 2048,
      "1a5b6538fa669bf9861e01554160b946c0d058d03955fffddf89d9c17827b84c"},
     {"presentation.rider.mike.race-tiles.v1", 3456,
      "f401d2ade33b05b5125f0862f5aa490f304cf66ab18c4f3bb141aa4120b70ea9"},
     {"presentation.result.classic.font-layout.v1", 5224,
      "63146ede94a8947ac632bf39d6f51b86f1cea31e70678e2fedbdeca14a993ca0"},
     {"presentation.effect.go-window.v1", 898,
      "33f19daed02ec2f968d1a1ba27675a4da794c96772af28edfc1e321e113c4b29"},
     {"presentation.effect.winner-window.v1", 898,
      "b6fddc697a55984d607220aff50ab465881297de3f94fd9e434c0c9be3d4df20"},
     {"presentation.result.classic.base-vram.v1", 41536,
      "c1f19c30beb818f2d65e524cb6894ce55c52f2cb6ada6a5aa92b47672b7e04f4"},
     {"presentation.result.classic.palette.v1", 216,
      "155799e64a81640cf5ad05a9c43b83062d0c32aa5249add78d522a5f55a00e18"},
     {"presentation.result.classic.palette-tail.v1", 128,
      "cd7b9fac3c3ec53d74450dcb28da0f53a09c927826ca3753ff047851c2b20641"}}};

const std::array<RequiredEntry, 30> zoom_required{{
    {"zoom.track-data", 50665, "db6770152e399f9d16fc6937b5d56588a8f67e825ae70d6578b77d36053fdd28"},
    {"zoom.collision-poses", 32768, "9d1754d38c20cb2900239557550211ab6fc23d9b0237e17b78fb29f3bf272c32"},
    {"zoom.collision-templates", 17249, "2f03a8cb985899436603ef36b233b28f7b4213e4ba106fca328a6b02cdb081c7"},
    {"zoom.tile-tables", 6496, "649b96ff43ef467fe57bac16eb876f96a1ebc6f0307a5270f97a7ce1f63a84c7"},
    {"zoom.tile-flags", 203, "11aa211a148bced17e6c1b63a19e6e9c71ff1f8fed83954613f2ea62b15069c4"},
    {"zoom.progress-transitions", 80, "8a90513f349bc7836513d3495a9bbb90563b9eb7a716fd4aff5b9a4c74fc83df"},
    {"zoom.pose-slopes", 128, "f6b1ea6a34c78336ca25449c8ddbd23e2417ef829ec09765e695f957cd714584"},
    {"zoom.displacement-table", 512, "27894923de2aaeb58ca24dedbddadcf0d4d154fbc61ea484e7c248d066e24e1b"},
    {"zoom.idle-pose-table", 64, "05d2af9f8c0d1d8d8dd1915086f4f4c58456510f3daf357dbd7fdd66e4a8031e"},
    {"zoom.race-finish-reward-values", 144, "1bb5d513974b4d12954688caa6e82b14e27410c6c23167b5c3bf49f41cb61e9b"},
    {"zoom.race-finish-reward-classes", 72, "f391cc910dfa30b93f2c134c133e11b88d622ee77c31f93bf3ce3143f2f7ec8a"},
    {"zoom.speed-masks", 9, "0ca19a78da56137e0926c4ba602d8041648a422b9cf5d0a7c3de4a30998cf58b"},
    {"zoom.speed-decrements", 18, "c1fab1d9aa1e691d34c1a78e8658cfd52b8334cc5bdec8efb23bedc672988068"},
    {"zoom.sustained-slope-coefficients", 128, "63f15f5116409177c4d41afb8f66fa1927f48989937af982891d8249040b2b67"},
    {"zoom.reflection-pose-table", 128, "2a20e85f9ec32303b9c77caaa20110f5dcdf03cd172fa90cd741b88b2acaccbb"},
    {"zoom.landing-response-matrices", 1512, "229eda89d8f29fd9daf2b2f9247e98e244a7683511d82bfa6abdc6d3ef69782b"},
    {"zoom.race-finish-poses", 1536, "c1aa7a9e72591a51bf701979359e199e453b35351634ed87049ff5ba721ec01d"},
    {"zoom.bg1-tiles", 25984, "e7be1f6ab6b4d1a10e4c4a730c030dd6234328b07c31514da9e9a6152fe49117"},
    {"zoom.bg2-tiles", 672, "ebb6921ca08d48bd76c02879a92647521524eb87d4ab4d4209fd7add1f37d949"},
    {"zoom.bg2-map", 8192, "a47ec4ddf2128d1126c299a8dea5fde497c326b01d6bd0ccc97483bf0fb904a6"},
    {"zoom.palette", 352, "874033ba4d54a86a3706e7ba92a17019d6df2ea1a9a3178de2b675ed8f6a7de6"},
    {"zoom.roll-pose-table", 128, "8d10c15244227029ba093c48a0d590553c9d48626e7f53a967a1f66a53341dcf"},
    {"zoom.roll-direction-table", 64, "670f922581331cb2546d768b7e2b94b6b3f0649b96ae1a46014cd9ebcf19f7ec"},
    {"zoom.roll-reward-weights", 26, "e27d32975fbc09805fa6bff958aaa3729167a1da354f7b6dfefa0bb838f104a2"},
    {"zoom.trick-combinations", 625, "a6424f66bbd354883f3bfe128487d6cab92add5651adbf969976733a353f0217"},
    // R-0036: $83:F0FF rider object sources ($20:8000 pointers, $23-$26 frames, $27-$3F tiles).
    {"presentation.rider.pose-pointers.v1", 15513, "089b5743b87cbcac9da71c8d54777f5a2ec829e30a177cf8499925701e53d8a9"},
    {"presentation.rider.pose-frames.v1", 124028, "f985fa7e6cdebe1418362efa9ee63f4a3a1307ef882be7eb210a7e4363157c51"},
    {"presentation.rider.object-tiles.v1", 819200, "a1e3f8925386d80d700f85306cc4c33e7322a1de3cbc4cf10219d35a2f8f55cb"},
    // R-0036: $82:833B, $83:EC2E, $17:C606 and $17:C614 rider look tables.
    {"presentation.rider.look-tables.v1", 594, "3ea809fc62e175660f9bb98c10b44bb80fd79ab98c7378c547790e3b6b986029"},
    {"presentation.zoom.race-palette-cycle.v1", 544, "a82bb2726a4bdbfd74483aae5261fb11e325ae35e88681125d9d68ad14f7a57f"},
}};
constexpr std::string_view two_track_rules_sha="5920a1301edaae061e530c8550758c90dae8b75d95e389f5c89fbea69308b236";

std::array<std::uint8_t, 32> hex_digest(std::string_view text) {
  if (text.size() != 64)
    throw std::logic_error("invalid compiled Classic SHA-256");
  const auto nibble = [](char value) -> std::uint8_t {
    if (value >= '0' && value <= '9')
      return static_cast<std::uint8_t>(value - '0');
    if (value >= 'a' && value <= 'f')
      return static_cast<std::uint8_t>(value - 'a' + 10);
    throw std::logic_error("invalid compiled Classic SHA-256 digit");
  };
  std::array<std::uint8_t, 32> out{};
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i] = static_cast<std::uint8_t>((nibble(text[i * 2]) << 4U) |
                                       nibble(text[i * 2 + 1]));
  return out;
}

std::array<std::uint8_t, 32> sha256(std::span<const std::uint8_t> source) {
  constexpr std::array<std::uint32_t, 64> constants{
      {0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU,
       0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U,
       0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U,
       0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
       0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U,
       0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
       0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
       0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
       0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U,
       0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U, 0x1e376c08U,
       0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU,
       0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
       0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U}};
  auto rotate = [](std::uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32U - bits));
  };
  std::vector<std::uint8_t> message(source.begin(), source.end());
  const auto bit_length = static_cast<std::uint64_t>(message.size()) * 8U;
  message.push_back(0x80);
  while (message.size() % 64U != 56U)
    message.push_back(0);
  for (int shift = 56; shift >= 0; shift -= 8)
    message.push_back(
        static_cast<std::uint8_t>(bit_length >> static_cast<unsigned>(shift)));
  std::array<std::uint32_t, 8> hash{{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U,
                                     0xa54ff53aU, 0x510e527fU, 0x9b05688cU,
                                     0x1f83d9abU, 0x5be0cd19U}};
  for (std::size_t base = 0; base < message.size(); base += 64) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t i = 0; i < 16; ++i) {
      const auto at = base + i * 4;
      words[i] = (static_cast<std::uint32_t>(message[at]) << 24U) |
                 (static_cast<std::uint32_t>(message[at + 1]) << 16U) |
                 (static_cast<std::uint32_t>(message[at + 2]) << 8U) |
                 message[at + 3];
    }
    for (std::size_t i = 16; i < 64; ++i) {
      const auto s0 = rotate(words[i - 15], 7) ^ rotate(words[i - 15], 18) ^
                      (words[i - 15] >> 3U);
      const auto s1 = rotate(words[i - 2], 17) ^ rotate(words[i - 2], 19) ^
                      (words[i - 2] >> 10U);
      words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }
    auto a = hash[0], b = hash[1], c = hash[2], d = hash[3], e = hash[4],
         f = hash[5], g = hash[6], h = hash[7];
    for (std::size_t i = 0; i < 64; ++i) {
      const auto s1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
      const auto choice = (e & f) ^ ((~e) & g);
      const auto t1 = h + s1 + choice + constants[i] + words[i];
      const auto s0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
      const auto majority = (a & b) ^ (a & c) ^ (b & c);
      const auto t2 = s0 + majority;
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
    hash[5] += f;
    hash[6] += g;
    hash[7] += h;
  }
  std::array<std::uint8_t, 32> out{};
  for (std::size_t i = 0; i < hash.size(); ++i)
    for (unsigned byte = 0; byte < 4; ++byte)
      out[i * 4 + byte] =
          static_cast<std::uint8_t>(hash[i] >> (24U - byte * 8U));
  return out;
}
} // namespace

ClassicContentPack::ClassicContentPack(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("cannot open Classic content pack: " +
                             path.string());
  bytes_ = {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
  if (bytes_.size() < 12 ||
      std::string(bytes_.begin(), bytes_.begin() + 8) != "URCP0001")
    throw std::invalid_argument("Classic pack magic is unsupported");
  Reader in(bytes_);
  in.skip(8);
  if (in.u32() != 1)
    throw std::invalid_argument("Classic pack schema is unsupported");
  const auto source_identity = in.digest();
  const auto rules_identity = in.digest();
  if (source_identity !=
      hex_digest(
          "a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e"))
    throw std::invalid_argument(
        "Classic pack source ROM identity is unsupported");
  const auto profile=in.text();
  const auto start=in.text();
  const bool two_tracks=profile=="classic.pal.crawler.two-tracks.v7";
  if (profile != (two_tracks ? "classic.pal.crawler.two-tracks.v7" : "classic.pal.crawler.dragster.v1"))
    throw std::invalid_argument("Classic pack profile is unsupported");
  if (start != (two_tracks ? "classic.crawler.race-start.v2" : "classic.crawler.dragster.race-start.v1"))
    throw std::invalid_argument("Classic pack start state is unsupported");
  if (rules_identity != hex_digest(two_tracks ? two_track_rules_sha :
      "70712c470db436ad95b02d3a6d51f737be7bb5b27689ca0d99a8297bac31d768"))
    throw std::invalid_argument("Classic pack extraction-rules identity is unsupported");
  std::vector<RequiredEntry> selected_required(required.begin(),required.end());
  if(two_tracks)selected_required.insert(selected_required.end(),zoom_required.begin(),zoom_required.end());
  const auto count = in.u16();
  struct Row {
    std::string id;
    std::uint64_t offset, size;
    std::array<std::uint8_t, 32> digest;
  };
  std::vector<Row> rows;
  rows.reserve(count);
  for (unsigned index = 0; index < count; ++index) {
    auto id = in.text();
    const auto offset = in.u64();
    const auto size = in.u64();
    const auto digest = in.digest();
    if (entries_.contains(id))
      throw std::invalid_argument(
          "Classic pack contains a duplicate logical ID");
    entries_.emplace(id, Entry{});
    rows.push_back({std::move(id), offset, size, digest});
  }
  std::uint64_t cursor = in.offset();
  if (entries_.size() != selected_required.size())
    throw std::invalid_argument("Classic pack inventory is incomplete");
  for (const auto &expected : selected_required) {
    const auto found = entries_.find(std::string(expected.id));
    if (found == entries_.end())
      throw std::invalid_argument(
          "Classic pack required logical entry is missing");
    const auto row =
        std::find_if(rows.begin(), rows.end(),
                     [&](const Row &value) { return value.id == expected.id; });
    if (row == rows.end() || row->size != expected.size ||
        row->digest != hex_digest(expected.sha256))
      throw std::invalid_argument(
          "Classic pack required entry identity is unsupported: " +
          std::string(expected.id));
  }
  for (const auto &row : rows) {
    if (row.offset != cursor ||
        row.size > std::numeric_limits<std::size_t>::max() ||
        row.offset > bytes_.size() ||
        row.size > bytes_.size() - static_cast<std::size_t>(row.offset))
      throw std::invalid_argument("Classic pack payload layout is invalid");
    const auto payload = std::span<const std::uint8_t>(bytes_).subspan(
        static_cast<std::size_t>(row.offset),
        static_cast<std::size_t>(row.size));
    if (sha256(payload) != row.digest)
      throw std::invalid_argument("Classic pack entry payload hash differs: " +
                                  row.id);
    entries_.at(row.id) = {static_cast<std::size_t>(row.offset),
                           static_cast<std::size_t>(row.size)};
    cursor += row.size;
  }
  if (cursor != bytes_.size())
    throw std::invalid_argument("Classic pack has trailing data");
}

std::span<const std::uint8_t>
ClassicContentPack::entry(const std::string &logical_id) const {
  const auto found = entries_.find(logical_id);
  if (found == entries_.end())
    throw std::invalid_argument("Classic pack logical entry is absent: " +
                                logical_id);
  return std::span<const std::uint8_t>(bytes_).subspan(found->second.offset,
                                                       found->second.size);
}
} // namespace unirally
