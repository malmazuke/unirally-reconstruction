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
        const auto v = static_cast<std::uint16_t>(bytes_[at_]
                                                  | (static_cast<unsigned>(bytes_[at_ + 1]) << 8U));
        at_ += 2;
        return v;
    }
    std::uint32_t u32() {
        const auto lo = u16();
        return static_cast<std::uint32_t>(lo) | (static_cast<std::uint32_t>(u16()) << 16U);
    }
    std::uint64_t u64() {
        const auto lo = u32();
        return static_cast<std::uint64_t>(lo) | (static_cast<std::uint64_t>(u32()) << 32U);
    }
    std::array<std::uint8_t, 32> digest() {
        std::array<std::uint8_t, 32> value{};
        for (auto& byte : value) {
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
                          bytes_.begin() + static_cast<std::ptrdiff_t>(at_ + width));
        at_ += width;
        if (value.empty()) throw std::invalid_argument("Classic pack contains an empty identity");
        return value;
    }
    void skip(std::size_t width) {
        require(width);
        at_ += width;
    }
    std::size_t offset() const { return at_; }

private:
    void require(std::size_t width) const {
        if (width > bytes_.size() - at_) throw std::invalid_argument("Classic pack is truncated");
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
     {"physics.speed.masks", 9, "0ca19a78da56137e0926c4ba602d8041648a422b9cf5d0a7c3de4a30998cf58b"},
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

const std::array<RequiredEntry, 32> zoom_required{{
    {"zoom.track-data", 50665, "db6770152e399f9d16fc6937b5d56588a8f67e825ae70d6578b77d36053fdd28"},
    {"zoom.collision-poses", 32768,
     "9d1754d38c20cb2900239557550211ab6fc23d9b0237e17b78fb29f3bf272c32"},
    {"zoom.collision-templates", 17249,
     "2f03a8cb985899436603ef36b233b28f7b4213e4ba106fca328a6b02cdb081c7"},
    {"zoom.tile-tables", 6496, "649b96ff43ef467fe57bac16eb876f96a1ebc6f0307a5270f97a7ce1f63a84c7"},
    {"zoom.tile-flags", 203, "11aa211a148bced17e6c1b63a19e6e9c71ff1f8fed83954613f2ea62b15069c4"},
    {"zoom.progress-transitions", 80,
     "8a90513f349bc7836513d3495a9bbb90563b9eb7a716fd4aff5b9a4c74fc83df"},
    {"zoom.pose-slopes", 128, "f6b1ea6a34c78336ca25449c8ddbd23e2417ef829ec09765e695f957cd714584"},
    {"zoom.displacement-table", 512,
     "27894923de2aaeb58ca24dedbddadcf0d4d154fbc61ea484e7c248d066e24e1b"},
    {"zoom.idle-pose-table", 64,
     "05d2af9f8c0d1d8d8dd1915086f4f4c58456510f3daf357dbd7fdd66e4a8031e"},
    {"zoom.race-finish-reward-values", 144,
     "1bb5d513974b4d12954688caa6e82b14e27410c6c23167b5c3bf49f41cb61e9b"},
    {"zoom.race-finish-reward-classes", 72,
     "f391cc910dfa30b93f2c134c133e11b88d622ee77c31f93bf3ce3143f2f7ec8a"},
    {"zoom.speed-masks", 9, "0ca19a78da56137e0926c4ba602d8041648a422b9cf5d0a7c3de4a30998cf58b"},
    {"zoom.speed-decrements", 18,
     "c1fab1d9aa1e691d34c1a78e8658cfd52b8334cc5bdec8efb23bedc672988068"},
    {"zoom.sustained-slope-coefficients", 128,
     "63f15f5116409177c4d41afb8f66fa1927f48989937af982891d8249040b2b67"},
    {"zoom.reflection-pose-table", 128,
     "2a20e85f9ec32303b9c77caaa20110f5dcdf03cd172fa90cd741b88b2acaccbb"},
    {"zoom.landing-response-matrices", 1512,
     "229eda89d8f29fd9daf2b2f9247e98e244a7683511d82bfa6abdc6d3ef69782b"},
    {"zoom.race-finish-poses", 1536,
     "c1aa7a9e72591a51bf701979359e199e453b35351634ed87049ff5ba721ec01d"},
    {"zoom.bg1-tiles", 25984, "e7be1f6ab6b4d1a10e4c4a730c030dd6234328b07c31514da9e9a6152fe49117"},
    {"zoom.bg2-tiles", 672, "ebb6921ca08d48bd76c02879a92647521524eb87d4ab4d4209fd7add1f37d949"},
    {"zoom.bg2-map", 8192, "a47ec4ddf2128d1126c299a8dea5fde497c326b01d6bd0ccc97483bf0fb904a6"},
    {"zoom.palette", 352, "874033ba4d54a86a3706e7ba92a17019d6df2ea1a9a3178de2b675ed8f6a7de6"},
    {"zoom.roll-pose-table", 128,
     "8d10c15244227029ba093c48a0d590553c9d48626e7f53a967a1f66a53341dcf"},
    {"zoom.roll-direction-table", 64,
     "670f922581331cb2546d768b7e2b94b6b3f0649b96ae1a46014cd9ebcf19f7ec"},
    {"zoom.roll-reward-weights", 26,
     "e27d32975fbc09805fa6bff958aaa3729167a1da354f7b6dfefa0bb838f104a2"},
    {"zoom.trick-combinations", 625,
     "a6424f66bbd354883f3bfe128487d6cab92add5651adbf969976733a353f0217"},
    // R-0036: $83:F0FF rider object sources ($20:8000 pointers, $23-$26 frames, $27-$3F tiles).
    {"presentation.rider.pose-pointers.v1", 15513,
     "089b5743b87cbcac9da71c8d54777f5a2ec829e30a177cf8499925701e53d8a9"},
    {"presentation.rider.pose-frames.v1", 124028,
     "f985fa7e6cdebe1418362efa9ee63f4a3a1307ef882be7eb210a7e4363157c51"},
    {"presentation.rider.object-tiles.v1", 819200,
     "a1e3f8925386d80d700f85306cc4c33e7322a1de3cbc4cf10219d35a2f8f55cb"},
    // R-0036: $82:833B, $83:EC2E, $17:C606 and $17:C614 rider look tables.
    {"presentation.rider.look-tables.v1", 594,
     "3ea809fc62e175660f9bb98c10b44bb80fd79ab98c7378c547790e3b6b986029"},
    {"presentation.zoom.race-palette-cycle.v1", 544,
     "a82bb2726a4bdbfd74483aae5261fb11e325ae35e88681125d9d68ad14f7a57f"},
    // R-0040: the channel-6 window HDMA family $15:8000-$15:D7CA, 25 tables
    // of 899 bytes (898 data plus the run terminator) selected by $83:E55C.
    {"presentation.effect.classic.window-tables.v1", 22475,
     "238ff3fc38357b359e99ae3da0647fdf61cb4a6a3680b83faa1b3a8a6a4a2cf0"},
    // R-0042: the caption table $17:C9F4, sixteen ASCII bytes per reward
    // event. Entry 0 is not caption text and an event id is a byte, so the
    // pack carries entries 1 to 255.
    {"presentation.classic.captions.v1", 4080,
     "1d5530ca0737ee3e87fdcc2ea26f020caac9d420cd824f29532c426b52ab6f28"},
}};
// TRACK-BREADTH part 3 (profile v10): the other race tracks a cold start reaches.
// track.NN.* is track NN's decoded data, tile columns, tile flags and BG1 tiles
// (R-0046 observations 1-2); scenery.SS.* the BG2 tiles, BG2 map and race palette
// of scenery SS = track mod 14 ($82:DC20-DD84); and the track name table
// ($83:9FFA). Generated from the rules file (tracks.py v10_new_entries).
const std::array<RequiredEntry, 90> tracks_required{{
    {"track.03.data", 63013, "ca5b470dea72bb2aa5c77ef5a5af62e06a5e90cb16401db329c412e10167ed1a"},
    {"track.03.tile-columns", 4800,
     "0b4188a23020d8e5079e123c909a068d5fabb6e985bd56b73b4db4e04ed785d5"},
    {"track.03.tile-flags", 150,
     "121fd0f557dd5e37285cfeccd078e0b42769e606fa9c858215d6cdd19a784399"},
    {"track.03.bg1-tiles", 19200,
     "3a2b1e9f1215bfc24fe84df2cd5b0de023f0fbd98384c8755ffcb9726ea52c23"},
    {"track.04.data", 52234, "4c05e160bf29160650eea327edfec640de42604b67208db798f4fbaef9a36cbb"},
    {"track.04.tile-columns", 6368,
     "8b09264dbe82613421ce4acf443f141d253b4bfdc4ac31e3f76ac7d7b673f304"},
    {"track.04.tile-flags", 199,
     "cc3faa9dcb83f163281354700c809f917a94443c00de9827a12aefac004838ce"},
    {"track.04.bg1-tiles", 25472,
     "500127487e9f3cef609d18296021a602a69b899ef53a9ea04dc4449f42c6824d"},
    {"track.10.data", 63397, "ac7a8c483c1db8201c241afde90497a58dcddf1cb084e3203ee127fa41efe239"},
    {"track.10.tile-columns", 5984,
     "8efe43a8c51936a1cbb68a144097bf5dc0bdc792f10fed6685d03eb60bc1398a"},
    {"track.10.tile-flags", 187,
     "c8ddb2e891c21f9cf7d17bbb580f70342a2a9bbafc2dbe6f00589d0c82255d60"},
    {"track.10.bg1-tiles", 23936,
     "8333764c348fed0b980654888023deaa89456f4c22c9c6f621eb795834d47c37"},
    {"track.11.data", 45899, "9eb49c560717454a9ece3a8cd4f1e5a07ac13a00b9d98fe18f77b0e6f9aa49ff"},
    {"track.11.tile-columns", 6560,
     "f4e2e8f77ed9b71e4f5cce22f90a479f4c44746dc8ebc3e8571689df3b99a347"},
    {"track.11.tile-flags", 205,
     "743f1ea85bd4d076efa033bfcb516d2a8139b9cbc07766f6d677c7f48d76dc0f"},
    {"track.11.bg1-tiles", 26240,
     "fd4e262f179e6e46d419e420f8e815013b5da6136f071aa80f1d89f686ad1c14"},
    {"track.13.data", 49156, "733690266fec100076f38b14dfb5814e74afbd360c3c18b489e489628be7027b"},
    {"track.13.tile-columns", 5856,
     "8baa3a1125d643c6f86ae584e061e8c5140bde91b401f99d896e5315528208d9"},
    {"track.13.tile-flags", 183,
     "946216b9dc997dd288ca3804d92a8acf64eca3ed23e0382bdcfc361fe625e007"},
    {"track.13.bg1-tiles", 23424,
     "d693c1e13964378853f68aca4c7d0fe4d18b3c550b21111755ab0eca11e86fd8"},
    {"track.14.data", 38219, "88e1a5b683e173659dab30791263bda1fb99c6125942d46d31c7ae2c3bac9f36"},
    {"track.14.tile-columns", 6528,
     "563e8e517ceef6446ee77a3fcb473cc2dacff94b22514c98c66c84031c6aeb95"},
    {"track.14.tile-flags", 204,
     "2d3a1800f1afc423d966d03a69c2a8a896706a6f59dcf5675a281aad5dbf181b"},
    {"track.14.bg1-tiles", 26112,
     "0b01421e918212ec4ed10bcfe27fdcc6bf5e1e4b461825c514e874b1d8be4b96"},
    {"track.20.data", 42663, "ecc4e63b0802aff3a364cdec9c1b32577dadbd1f5855e9d112d365d4f6dd71c9"},
    {"track.20.tile-columns", 6432,
     "b13510c422cd024f6db848fe2e098bcd2f798df29cb67b74bf08d5373f40e1ee"},
    {"track.20.tile-flags", 201,
     "b5b32197e5db375e863e27024e60e745b77f2cfc50d593ebb88dc96454b367e9"},
    {"track.20.bg1-tiles", 25728,
     "5125292eb60ad5680ce3160c2684779a91a1b0b8d1103cc1107245c3662848f7"},
    {"track.21.data", 46893, "eff789427c19a4b5694c23b53f10c29c5de4ef026091267ab7127f3e485ca5c7"},
    {"track.21.tile-columns", 7008,
     "1c632e46015ed05299144be3183d56609fee3a583664e35b5eac9da247a1613a"},
    {"track.21.tile-flags", 219,
     "f2b84cd44c07b2f1dc07df2cf098fa78eecddd7df951b1ab51ac508ffd81ecf8"},
    {"track.21.bg1-tiles", 28032,
     "d75be08f3f936ec0592ab31d380b7edc4e76c6f7325175d844b13ec4bd9b7c60"},
    {"track.23.data", 47906, "c2a4ab9d80e8e0011ec46077848187d14da8bd9928aa62c81842a19c73a1f8d3"},
    {"track.23.tile-columns", 5696,
     "4e30516a79b2383bdfe4b44c269c3a7d13139ccff2a750b78d8f14020bd7e38f"},
    {"track.23.tile-flags", 178,
     "6edd1d3d35147dc49f77ee91f18a2658f204652c023d0b99a996c516a0506dc7"},
    {"track.23.bg1-tiles", 22784,
     "4430cec00d03c40ddace0eba3b83430b74c4a29880e64a2d6bfed1ac4145b43d"},
    {"track.24.data", 47078, "75caf67770db9698ae055baa02a83f32e75ed2f69431e3505f6c203bc96b7b40"},
    {"track.24.tile-columns", 6176,
     "2ecec38b34f92bf8c607f27e80e06d5af811ec89c55de3548f46c54044aae4a2"},
    {"track.24.tile-flags", 193,
     "1df7c37f597afe316670cd0925a088a64ed50635f8bca39223d6a783a41ae4f4"},
    {"track.24.bg1-tiles", 24704,
     "3beee9b9bb5a9362d3c8e323c0da9eeccd9e8dcd2b59d4ed8c567e388b0035e3"},
    {"track.30.data", 49932, "ff2863f48ce66f06a62b0f754da836220aab164658d38ee9516aa2c4d819a451"},
    {"track.30.tile-columns", 6912,
     "c79464f14c7f8e85b997c6e31bd27ff71af2d348a10b9c2d85f4f6c282f4392b"},
    {"track.30.tile-flags", 216,
     "baa4e5e8ce5200be2a682d94a53eeb3bb05f74e7a7259b651eef01cfc5a5f151"},
    {"track.30.bg1-tiles", 27648,
     "55a8f0e0990828161d5a10a74979862013d88d8d1fc30c11ca25f594c2ea0b98"},
    {"track.31.data", 52171, "5d039870a8c51150cd7d0d0abbcf398fec99fd4039d5f151657e19c5cfa3ccf7"},
    {"track.31.tile-columns", 6592,
     "1ffe8e43a00de777aedf75d8ff63524534f7d2d71d16b6aebf31c589951ebe42"},
    {"track.31.tile-flags", 206,
     "c7e7d432408657ff4aea5e47204968bf965e8c9f11605405cfa5df6b685bbf77"},
    {"track.31.bg1-tiles", 26368,
     "dd13c2a83549f4c8de7c1b6098940559fbf8597abfe570644bd05dcfa07dee99"},
    {"track.33.data", 48099, "7d111873717e65cd94afb5713414ee25960d62fd4c0f0ea51b9f4f36474a40e9"},
    {"track.33.tile-columns", 4608,
     "04809281c4f27fbc7544f6a0416f1a46f423913145b1697f2c88182d63634f79"},
    {"track.33.tile-flags", 144,
     "a15e8c2e02833e9c07acd42dba72756164e55bf24fd068fe5b88e83d1168256c"},
    {"track.33.bg1-tiles", 18432,
     "e28cdc2228bfe146119973074881e2210ef2ae58f123ac3f4f2d27cbef73452a"},
    {"track.34.data", 43340, "09cfc8d7d4ad01a6e8296c844336956683c7e27b1acc47b675da7f71d1a09c91"},
    {"track.34.tile-columns", 6976,
     "ddf4d98f88e4aafdd5a95e50eeb996653c9606e1e2aaa18bd75c7943aa6f634d"},
    {"track.34.tile-flags", 218,
     "2a7e72a7581e29429ff79e1f153bfde4d3aeac2f63713bb00c2b0c3b2d0bb576"},
    {"track.34.bg1-tiles", 27904,
     "59ccf2f01f748d9335baa8168da3a5a6158b54ba523121b0a458fd496a27cc9e"},
    {"scenery.00.bg2-tiles", 992,
     "d50aaa4efde3d4b5eec805a69470faa368596087f5588d7d8940c95226bcb8a2"},
    {"scenery.00.bg2-map", 8192,
     "574a44e71c96b210f5613f9b42e2de80e69e1aa64aaf59b8a3b3952f5f30d6b5"},
    {"scenery.00.palette", 352, "d98f7dfd1f0cd056aca52f2317a80609854738cc7b7e1c1e10f69ccdfea150ac"},
    {"scenery.02.bg2-tiles", 448,
     "d8ff1c6a7327c0ce71ec8775584a98de8a693801e1f8d07d7c6051ceb047d5e5"},
    {"scenery.02.bg2-map", 8192,
     "b4bab348fde46f1facddd4e0473e6f1111d9cad077b53569ba6c21171d6696de"},
    {"scenery.02.palette", 352, "af5bcf763dc663b104dda16d24142ed8a2e1d1d199ec351d25ee6eef9f8d2b5b"},
    {"scenery.03.bg2-tiles", 576,
     "36143cec13c6c0245193c3f26c82239ce88cd57582c8851af95c96f78d75b3e9"},
    {"scenery.03.bg2-map", 8192,
     "01aa2c918c07392929f4431dd90cb2f34b052b0a1bbacaa16bf89e7002615551"},
    {"scenery.03.palette", 352, "dac9e732180990b52215a285507d5959873fc7c4b354afa70b6bc36da1b2a8da"},
    {"scenery.04.bg2-tiles", 896,
     "e2a64d3e256e1717cffa1c5b834c0aa0a1ea57722c0c7b7457ef8515b6f5d1d6"},
    {"scenery.04.bg2-map", 8192,
     "20b749cb6524d1f35f52d8410026f964891d33d7e3e50b993e3257463e6538b5"},
    {"scenery.04.palette", 352, "76a94fa96c09600faa5f345f6a1447d4511cfb510eb38932461654dba37e03dd"},
    {"scenery.05.bg2-tiles", 1088,
     "2731c6b238c574dce14a93dc06cc8f0274abf66805a2dd6ad3e3820a5e1ec848"},
    {"scenery.05.bg2-map", 8192,
     "10f227026900d8fd9c3651aad43c66837b4239aee9987d2a3bac7af1a4af532c"},
    {"scenery.05.palette", 352, "2c0e3e015ddaf77ad5f69b777974e1b5e6f71187e76db7f34dbad3c1a18d74ae"},
    {"scenery.06.bg2-tiles", 1408,
     "20ee2b87b5586c75672730d3ffea75907a6ff843bbc65e1eab600a0569861895"},
    {"scenery.06.bg2-map", 8192,
     "139eb0eac3cc23d411fc4f125bf68159e8c68102af94a54315789e1a1ed64ba8"},
    {"scenery.06.palette", 352, "8424da4771f280f18241353a97b7d88bd68ed91e7697b794c2c5f56c8136ffad"},
    {"scenery.07.bg2-tiles", 544,
     "dbace4d8148e4d60eff46a3169123f25448c797906caa1314f5cb9fdc0b36a09"},
    {"scenery.07.bg2-map", 8192,
     "a222e8b779063bc133e32aabf771a0fee6577114bf3ec7e06ca742b93cd2ba40"},
    {"scenery.07.palette", 352, "1f48cd833cfde7ad3afd315769b4ce16c13bcacc78ea585d2e187287892bf1d6"},
    {"scenery.09.bg2-tiles", 576,
     "64aa3ad7a7af40c5a24ef9cfe648737f40b34599432ab09828dc24894a02f1d7"},
    {"scenery.09.bg2-map", 8192,
     "1a0f3b9613a6d728c3bd959772ee21c3f5cefb1025be1e310b0a11e2ddaac3b7"},
    {"scenery.09.palette", 352, "31d312b66be7dac0a43226944f2703cb9ec9e85d94c139aa7f2f9360bf407cf0"},
    {"scenery.10.bg2-tiles", 480,
     "765720adc364b5afe39c76ddc42db5f245832e78af049f454038a04bd06b0e54"},
    {"scenery.10.bg2-map", 8192,
     "64d36cc386bb70fafe0d63cf33da441f739e8e40c65539301d930cbf8c30afe3"},
    {"scenery.10.palette", 352, "2b40a219bb2a080a7c41eab9572b43243953e5027716113ed39eee5cbf462c0d"},
    {"scenery.11.bg2-tiles", 1696,
     "09e6c0193a6ce4402bd5f845deef9c948961a18d65246150781fdbfbe49582d6"},
    {"scenery.11.bg2-map", 8192,
     "25103f7ddd6e4eec973903fd0c2e48e9406232b5d06912588abd4b45baf3db9b"},
    {"scenery.11.palette", 352, "9347a75485520c6741dab1b479ed79cec308eb2ea0f893aef28f22a49f0b2f4b"},
    {"scenery.13.bg2-tiles", 960,
     "cd6f467d454dd743fa4e960867a4a3fb1c3ee2330459048886bbf5f044a8be0b"},
    {"scenery.13.bg2-map", 8192,
     "aca84d66d574097846db40bb0a5b5ef4a07a96e05717b249a0da65d571d1cae1"},
    {"scenery.13.palette", 352, "c98592fd78dba9ab9a13e4b0586722103625f1dd4279202e02887cde17140186"},
    {"presentation.classic.track-names.v1", 382,
     "4557672dceff27e910ea76b428623162c0e807831d8a2c537088e7f6c62277bb"},
}};
// SPECIAL-TILE-RESPONSE (profile v11): the corkscrew tile's two signed height
// tables, $00:8088 and $00:80B8, 48 bytes each ($81:891A-8947, R-0047).
const std::array<RequiredEntry, 1> special_tiles_required{{
    {"zoom.corkscrew-heights", 96,
     "4529c691946c40e5d14b00445ed4d888e5d34fbf94283b874e3536cb9a90a7b0"},
}};
// TILE-PAIRS-8-12-26 (profile v13): the loop tile's x step by loop step,
// 17 signed words at $81:834C ($81:8462/$81:8476, R-0051).
const std::array<RequiredEntry, 1> loop_required{{
    {"zoom.loop-offsets", 34, "aeeccf997e22c9ea317e86148e330dda9d30a89c188c48d5420c1c55db71fb86"},
}};
// HUNTER-EFFECTS (profile v14): the HUNTER tag effects' blink pattern, 64
// bytes at $83:D3BC ($83:D316, $83:D395, $83:D44D, R-0052).
// And the HUNTER opponent's sprite palette: asset 26 (character 20 + 6), which
// $82:DDB0-DDBC loads at OBJ palette 4 where the other tours load asset 23.
const std::array<RequiredEntry, 2> hunter_required{{
    {"zoom.hunter-blink", 64, "b4adbefef0d7b71c40b0a806002a5cf5598a79b5e14992030c3d498f6955c93f"},
    {"presentation.classic.hunter-opponent-palette.v1", 32,
     "a8d2de3b96ccc5a277cb3ffecaa92006913197e0887ccb76f27a7a25d41d6d8e"},
}};
// FRONT-END-MAIN-MENU (profile v15): the boot screens', the title's and the main menu's
// content: seventeen assets of the directory at $82:B332 and the tables the front-end code
// reads in place (tools/unirally_lab/content/front_end.py, R-0054).
const std::array<RequiredEntry, 23> front_end_required{{
    {"front-end.asset.001", 32, "73c8f16cc7220c5806654becfe758100ffef69564fa33de10fe7ba88c1bdbd6a"},
    {"front-end.asset.002", 32, "fabc24409856be6df45a345a13bc87dc2b0359456143bc11f31a9d91f4fff73d"},
    {"front-end.asset.005", 32, "5cf20ec26943ebf74b9c97cc7051a344febb879d2ea29d8763a8988377cc334a"},
    {"front-end.asset.027", 512,
     "23e5ec214c22a7acedbaabff18b45bf10ed0a644d3c3aecff18a82041988005f"},
    {"front-end.asset.028", 32, "4260f86adc609268c4819fe36eaad1e112e2a9234f31986eb1a0dbbb9ad40e8e"},
    {"front-end.asset.031", 32, "d7410a2f53e08ecc998efde57354c2151c68e78bb0296434a20ab0c69ac34a67"},
    {"front-end.asset.068", 8000,
     "62109e5b77150a412bb6361fe3653e39fdc1184c5aed27161f90549bd3ed4799"},
    {"front-end.asset.069", 13568,
     "2c8fe1823f71d2affad5fc345926c6ddbc300635b43fc5525718e9882c31a2a5"},
    {"front-end.asset.070", 8960,
     "9f153a74089ead31e5c84fbe0a69c3ab4d0abe0cdfb06f60ece0a3a48cd15a5f"},
    {"front-end.asset.072", 36672,
     "ec64d417090f8f489caee5fed2fe347bbd004e9f902bb8921c8c35e0c1420d9b"},
    {"front-end.asset.074", 5120,
     "dac866b120f40dbc9fea643f7520a96ea333aa2d7f6892c19959711adc808310"},
    {"front-end.asset.077", 2816,
     "5d4d58fe0ac75520a1a86d28026ebb608b630ca9b58c3e458c48f3507f6fe218"},
    {"front-end.asset.078", 1856,
     "a97c2988f00f8ef70a249f82c96c126f9f6721bdcde6a7bff554bc48a5e4d575"},
    {"front-end.asset.080", 1920,
     "123014b49256f0458741778d48f1fb962036f793310a2250f5ed15c487345119"},
    {"front-end.asset.088", 8192,
     "9c229230f22ad540c88de56aadc78f7a7bf967561ccbf28f74a468d374497a47"},
    {"front-end.asset.089", 8192,
     "e90bb1d27b1fd30d53bc697fe2750777e7e367ccff5c49e3789a6df0f7042ba7"},
    {"front-end.asset.091", 3072,
     "37eb293068522399f2bfcbff576eb3b5cbd3d58e15acaa4683cc696016e8f51e"},
    {"front-end.base-palette", 216,
     "a933e35522e24c7a08daca7caa10305cd7635521850ef3450dfad00124f36251"},
    {"front-end.character-table", 256,
     "73159dcad6d673d652b68229518cb4166aaa1f0a26d6e90643973bcf653a110b"},
    {"front-end.main-menu-text", 36,
     "b6c620d85b9abbb5b4901af81153edf8c07e3ae9b83847f2d94dcf910c504ced"},
    {"front-end.arrow-frames", 16,
     "2d4174c62a9a7e66cdb3f59cd4738609510ea84469b4fea53a1feb44fd696cd0"},
    {"front-end.menu-arrow-columns", 5,
     "ba56be3883c239ca143d7c41fe5150b4bc3549b193742dcbe15dab7157f00880"},
    {"front-end.cycle-colours", 14,
     "7c6f09ee6029f31598b51a183b9f0f1927dc014108098c1c71c5d2060c31cb43"},
}};
// FRONT-END-1P-SETUP (profile v16): the rider menu's content: the riders' sprite palettes
// (assets 6-21), their name records, the menu's title and the decoration animator's tables
// (tools/unirally_lab/content/front_end.py v16_new_entries, R-0055).
const std::array<RequiredEntry, 19> rider_menu_required{{
    {"front-end.asset.006", 32, "36d2c1daa25deffe45c2f856b738b936e99f5e623b6639c6d4bf810eb3abc0b8"},
    {"front-end.asset.007", 32, "346e7faeeb6c4e2ad101d22f464e9fe9077e0ba349da87e6b7efee4c1a04793b"},
    {"front-end.asset.008", 32, "2b1984513a60c382e74f917d1f1a6ea5a093dbc0f3bc8a704384beab1933d8aa"},
    {"front-end.asset.009", 32, "3f0535a59be05d3c3fd6de204f5350409235f3248c151b10bcbb3a2c5d670af6"},
    {"front-end.asset.010", 32, "402291ffc3dcdf0857609ea3baa58069e04c4fc0bca3a752708bf16f50a6e770"},
    {"front-end.asset.011", 32, "7c11b2ccfaa2f6de0457aba9745439222426998799c33d5cd21ab0779006ec38"},
    {"front-end.asset.012", 32, "e798b3d6b4c50fd93990c05286e9ed46ce3fb467e101f26222d0cd82d17fe146"},
    {"front-end.asset.013", 32, "72fd819f4f61d2c472455e0b23fac0e1af6548cd16e097340ee7047fbce99511"},
    {"front-end.asset.014", 32, "f4ea7911d64d4aa17d21b788c5f9ce00b259a6d48eadec99c4da80691adb8e9d"},
    {"front-end.asset.015", 32, "11852e42eb52de1c32b8df636a7c9d0054aae3bb19099d49652bc98039356fd8"},
    {"front-end.asset.016", 32, "598eb272c223e159a28e91418b38045ad21779b4d0873e894d5b17f970a09e01"},
    {"front-end.asset.017", 32, "ccff0947e4a7f12d19709fd817f658dc530fd59a587d2aa546f322d51e447892"},
    {"front-end.asset.018", 32, "ed56ebd993dc549e71a5ba6aec24ef03098c7353b9eaf10655df2601b797dc9e"},
    {"front-end.asset.019", 32, "bba899c079335e75073b4503f5a60a6f01ebeaee829e1b789c0f9a8405437a6a"},
    {"front-end.asset.020", 32, "923886495d22f626c8ad2b3a534657a6ef2162a8c77efa4fb24857e560eea452"},
    {"front-end.asset.021", 32, "e6b7ccb29d361d8c402a5f10d5d03cca91656faba474a61e03ff03656426a229"},
    {"front-end.rider-names", 352,
     "8151e9a13f8d35e31de85d4efad1dbdedb28a4eaad3cc492f7207eff1b2c66c4"},
    {"front-end.pick-rider-title", 16,
     "18cdaafcec07a48d95ac5a666d882c1419259c5bca93854485ac9d17f0c18e97"},
    {"front-end.decoration-frames", 76,
     "91dd4557d6f9b28a69d69720e2ea4c1e5aaf71247bd7ba4d439d834802785c3d"},
}};
// FRONT-END-1P-SETUP part 2 (profile v17): PICK TOUR's, PICK TRACK's and NOW PLAYING's palettes,
// object tiles, text and tables
// (tools/unirally_lab/content/front_end.py v17_new_entries, R-0056).
const std::array<RequiredEntry, 30> one_player_screens_required{{
    {"front-end.asset.022", 32, "dc19e3f415349382ca580033df6a25a9ccee6911d702ee347bcb718a40181c25"},
    {"front-end.asset.023", 32, "45fcaec6849e38e57046a444140508244ed1fc53d8ed521990a9176dbbfb0a56"},
    {"front-end.asset.024", 32, "e1f7bd6ca99d90433586815037afe9b5bc60e23b6e34dd4b6821a09d9d08d1c3"},
    {"front-end.asset.025", 32, "49158e78a8479bc21c65bcfc8b9f6277fa23306a8e74f8abfa3f7b800c85a33f"},
    {"front-end.asset.026", 32, "a8d2de3b96ccc5a277cb3ffecaa92006913197e0887ccb76f27a7a25d41d6d8e"},
    {"front-end.asset.032", 32, "fb3ebb50272c0b14248c796f0d4fb5d274631395abda7059f12d7fa351a5785f"},
    {"front-end.asset.033", 32, "ced41ab2eeee8a9afdbf3fc993d0ddf7a52f3969aca08aa4786e527cb5a328f2"},
    {"front-end.asset.034", 32, "c8f6a64006fa59e874cb0346dcd3ff9d53dc0c337492a4ebee358f85d49fd67b"},
    {"front-end.asset.035", 128,
     "5dae33d15f7e6d183f05242207b8909bc2a3274f8f785e17f3c75f102d8ce89b"},
    {"front-end.asset.036", 88, "f436724f2f119eb265e224a1c63ed66eb535309bd0a8e79e6004c4e4c4b1efe0"},
    {"front-end.asset.037", 32, "e6badb0c68232da353c51a90ee7daaed43cdd37ce01d05d681a9037fee8bbacf"},
    {"front-end.medal-tiles", 3072,
     "37eb293068522399f2bfcbff576eb3b5cbd3d58e15acaa4683cc696016e8f51e"},
    {"front-end.track-menu-tiles", 3072,
     "f5804175493de79bd303aee653b914a1da28da68e6d84eab62edd86956b05759"},
    {"front-end.tour-menu-text", 106,
     "0c2e16fc3839b042fe98c86f73ef83141b5f2b5386c4dad0d28c6de83c0564a8"},
    {"front-end.tour-badge-places", 18,
     "51218aaeeab310273ce39ae1a8e01d24b72df1ef37fc708dba34996a15bd632c"},
    {"front-end.tour-levels", 10,
     "5cb36656e6d0b51da87e5092c974ae2ac5748076076ab4ff4d6033517892b5bc"},
    {"front-end.tour-arrow-targets", 40,
     "5b7b258b440873370a858547ed36cbc2ac16526d883610caed6c9ce01b610ef0"},
    {"front-end.tour-badge-pictures", 30,
     "963feda1f6c48aab1f267234b5df070ed33821cac0bc2b2df9a8acbbab977e60"},
    {"front-end.medal-places", 36,
     "c71438d4455b8361466f0d487d8334ffa40b5b27f00aa9814b0374d2398b8b55"},
    {"front-end.medal-attributes", 4,
     "f92831233ca28644a831ecbb62f986b1ddc17e1a23ec5dcef17b4d8d8e75a6cf"},
    {"front-end.tour-names", 80,
     "df9785d22e456f1e3e73f6270a50b09478d0e5f0396ddb4ed7d92eb6238d2f84"},
    {"front-end.track-menu-layout", 35,
     "83be7c59633a47597c0091c04f4247173822631ec334e86214f6b23178d02add"},
    {"front-end.track-menu-text", 55,
     "c1473ec694fa1b1e8f95481a53e050b9b3228d05be30e4fd9a575cc7dd4911a8"},
    {"front-end.medal-words", 29,
     "3539e8e0ac30b4e12c584f1f1b704ec2cc35b5743b94cb7c850e3487585339fc"},
    {"front-end.marker-tiles", 21,
     "4490a41a4b6d6b61a7a3123834cd24abee3799a2b641235b2cc439337e5018c9"},
    {"front-end.race-kind-words", 40,
     "5e8c62d95312baf99c8b6352363b8e53239c4dfac85f264517b5883da7e26706"},
    {"front-end.now-playing-text", 136,
     "20ae7da9f44789b924c0e1b1c6f2c89179769f7f44642502ed756ffeed5e3c98"},
    {"front-end.time-words", 18,
     "9158849d3cadd0331810b06b8b3a1c90565505d15052b406a8d7e1758ddf87d4"},
    {"front-end.laps", 50, "d9eb1969e8457b372f7ec775b03aefd69a5be9d6d5d5114b6805807577c8695a"},
    {"front-end.qualifying-scores", 60,
     "ab4af8f8d321a129369d324aad5ea51d56dc07c11401c293879e7afb2eb18554"},
}};
// FRONT-END-1P-CONTINUATION (profile v18): the one-run result screen's text and trophies
// (tools/unirally_lab/content/front_end.py v18_new_entries, R-0057).
const std::array<RequiredEntry, 2> race_result_required{{
    {"front-end.result-text", 84,
     "746239efad431a1e7f7a1959af9fd8dc5635cb099cda11d12aafe495ac8023e8"},
    {"front-end.result-icons", 12,
     "932debcc0ad000f6b980d5fe057dd9e0519e36e4ef78af87615ac7dc39cc88f4"},
}};
// LOCKED-TOURS (profile v12): the race tracks of the five tours a cold start does not
// list, and sceneries 1, 8 and 12. Generated from the rules file (tracks.py v12_new_entries).
const std::array<RequiredEntry, 89> locked_tracks_required{{
    {"track.05.data", 47619, "24d5b6163c456da21a8027f6cbd3fbcf87ad54726b96d4159cf180d215431924"},
    {"track.05.tile-columns", 4608,
     "1ff764587690707a29d88811970122a0bab1db23cbfdd914a06d93b20a002bf3"},
    {"track.05.tile-flags", 144,
     "a15e8c2e02833e9c07acd42dba72756164e55bf24fd068fe5b88e83d1168256c"},
    {"track.05.bg1-tiles", 18432,
     "74eda420a56801bcfdc03a1dde9f72a19a5c7d6cd358f44de8f53290b8243c59"},
    {"track.06.data", 48168, "b783de52aac4591ab7d5a20df19a6f6760cf9b6b348099fa9df64b2f2313c5a8"},
    {"track.06.tile-columns", 6368,
     "d78eada280eef6ae013e1276c2fd421be4d8cbd10538ddc15bd0eabe979e9067"},
    {"track.06.tile-flags", 199,
     "29da001bc2ffc18cb8fde098a2f54ad8d3288f8bc7a43dcdf6aa1c6fc4d4a390"},
    {"track.06.bg1-tiles", 25472,
     "6676554cdf399836de7387df9c0e0eafec3316e02a1bbc2d3385ad50e22e027b"},
    {"track.08.data", 56372, "2919362dfaaf0759f3fe3e3caba448579c6b3e65997274ba0d672723bbdbb376"},
    {"track.08.tile-columns", 6912,
     "d2974b9780bf7591953a31f8c0f8b65a33c42ec070ae0d4b87c4a93280655e3c"},
    {"track.08.tile-flags", 216,
     "18ecd22b62eb06b937cf3b87f5411db1c65dd5b35069437abb2857128dd817b6"},
    {"track.08.bg1-tiles", 27648,
     "a73daaea1c194dadd5b1383daf8c3efff987d3df91ac1178ba71935b061b59e7"},
    {"track.09.data", 45036, "52b7a307264af08c5b3b1972e5ecc79773c0a02360d0fd6dfddc6da47baca11f"},
    {"track.09.tile-columns", 6912,
     "7cd75c0d8476eb5916c6e06f61548df8dc5f89b678629e259724eefdf80dd13e"},
    {"track.09.tile-flags", 216,
     "a823334c96e825d7df57f04e3270b6653b98aeb0899acc9e7a14fdc9cb216ece"},
    {"track.09.bg1-tiles", 27648,
     "35c5cf80049a6c3f6348e3429e8642c8fed20361a7128591f2b2225d8a27a1bb"},
    {"track.15.data", 54382, "ae55f88b7b248e0e2334f9807c42e6b7d716574eaf4a688990508edfed281314"},
    {"track.15.tile-columns", 7168,
     "6b04948b316d1cf0624139a2d2bd34f793bc5e5d7ca8d365566b35db4efdce90"},
    {"track.15.tile-flags", 224,
     "dbdc139a40ef6c1e0a26c937c2a8945f8cf4713eb92ba80e8077fd6cef9d8090"},
    {"track.15.bg1-tiles", 28672,
     "3c64d644b052304cfd8647a2352d6ac97d51cfb3aa04af80d53ed80ac35391ee"},
    {"track.16.data", 53331, "d27621f9bfa8f0dca0fff18dcf90ed32caa14cf7e99a85fad39fde2c171081fd"},
    {"track.16.tile-columns", 7936,
     "3cf63fad4573cceac6d1d010e277340753dc7634d6255509af4b62a129cb467d"},
    {"track.16.tile-flags", 248,
     "c957d488fcdcf19b757d8ba4860707f8374f905a7c115b689c93114e851e94f4"},
    {"track.16.bg1-tiles", 31744,
     "507ac3f6ecac8c4557cd6377bb6da23d4f972481e9fc67aa494dff98bf9ee0cf"},
    {"track.18.data", 49258, "31c75fee7b265c5af2d7190b39a7350ad835466ed154516ff204f27db74430df"},
    {"track.18.tile-columns", 6976,
     "f0e7c2c31f021253f968f7052f1b51a9351d74a4568f5079bceef37ca7c78e4a"},
    {"track.18.tile-flags", 218,
     "be2f048ad3825a5f2cd5c39f3f1074946d89f6f13609090b4f6716af047d3ef2"},
    {"track.18.bg1-tiles", 27904,
     "e4f851b47a7936497fa42ef386c71e31c182aac25a417738ba90ed672b9028cb"},
    {"track.19.data", 52178, "d107bcbfdfd76e795e5eacd297f19fb0c95454260f87e444261cf8018b976ebb"},
    {"track.19.tile-columns", 7712,
     "068dd6fd62d932518193bb709540e0656389bdea62f23b073183be63b3dc00e5"},
    {"track.19.tile-flags", 241,
     "5cfb1aecc76a0414278875bc40cfa680d4166c186d9f7bad72b27888f04de7a7"},
    {"track.19.bg1-tiles", 30848,
     "25b13af1caf67f192ed4580515ca240de61d5eecadf96ed28bd7eff7a9eeb442"},
    {"track.25.data", 50764, "7733352ca34f904c40a18c7be997ed0afbb5e58cac03dff5d4125a8b0ffb3ae6"},
    {"track.25.tile-columns", 7104,
     "a11572bf5ec4906b2f387dc80ab629889d5c5c22384a31e844e252738c60c721"},
    {"track.25.tile-flags", 222,
     "85f5a7ac55c6383a72b1ec27216f0b35c41b6ed9ea1fa2349ae96558f9f25812"},
    {"track.25.bg1-tiles", 28416,
     "18e45e85911fad306759335f7ce7942770a871c1400d6ad239c7b060f95e54e7"},
    {"track.26.data", 47215, "3cc9fae1426f3d8cf5c82ff8f0292775786124c61a4a9474c490de797647b18e"},
    {"track.26.tile-columns", 6848,
     "aed9591c58cc4e67c760cd1ebf29dc0518891a678925feb9dc8fa9d08346a0da"},
    {"track.26.tile-flags", 214,
     "c73ea107665158c170f807dc06f7f45946cf43c8a672d867b4fa6a79513ef997"},
    {"track.26.bg1-tiles", 27392,
     "e62d31606a9fd9fe8ec385aed8517efd219439d999a41f2f62c8280422b8c939"},
    {"track.28.data", 53001, "c072d1604facbd01f48ffb380849bbfbe33dd36cf56c08bab15b0ee5d701ad42"},
    {"track.28.tile-columns", 6720,
     "3215a3369b510fdb8b0d56f1ff8677fe3f0c5fd0970ca0bf1546945f6856622c"},
    {"track.28.tile-flags", 210,
     "0f0888aba037dcd862bb689c76dcf7037771af748a7b0e90028362324f0d8ada"},
    {"track.28.bg1-tiles", 26880,
     "a2976093e84785e673cf2e722385241215ccad8d5e6c7b5a2b03541d663bc268"},
    {"track.29.data", 44483, "7c27db5e9793287f142a633f8658f8a9c3f5a0a672779a1ff71e2900a3beeceb"},
    {"track.29.tile-columns", 5824,
     "57f139275d7e899e37421d6bdd84697b321a833b3b844589e4081b34f13b8677"},
    {"track.29.tile-flags", 182,
     "03fdc6f586abc5f273dc543c1b49837dc8c97a0b7d35523761d6faedcaac0e5b"},
    {"track.29.bg1-tiles", 23296,
     "6c898432f52a7c78dfe7a383d04664d19ff3ec390314634830ae269337047413"},
    {"track.35.data", 57267, "5c5a441facd11576ccb2037b920c886dc789f6f733327f7df1f22d727db59e8a"},
    {"track.35.tile-columns", 7680,
     "4f542d6d1bb53c7e85c59af8b57dc76aeb93f6b474a75227042d0b966be558bb"},
    {"track.35.tile-flags", 240,
     "d945b356b22b1f058f72822c7a31a51b7937e8d936b63181ec1ae4c350f26a38"},
    {"track.35.bg1-tiles", 30720,
     "a07ac9690e5f861b36dc32e98e968d797fd14f14259c51fece3e2652fb0986ef"},
    {"track.36.data", 41512, "f2cbc9e4cb64f994801435a07b591b6b4012de8d00fd36345c7d88b3f44e61f3"},
    {"track.36.tile-columns", 6176,
     "840720a5a680dde625beeadbec685085e506a8b4e826aa53bb377704a847f5fc"},
    {"track.36.tile-flags", 193,
     "951f7eec193436227ecc42a63ba7ce5e9622abd26fb6ef0f95e7df45b924244a"},
    {"track.36.bg1-tiles", 24704,
     "1c1aa22eecf3558232b2137d42f15a34afe3acb4bf4cf13fa76382035ea0f799"},
    {"track.38.data", 47242, "75d377e405bd2c580c3559a403409e033d849aa78100d5d3491c78b16e998932"},
    {"track.38.tile-columns", 6752,
     "ef8d66022f0357815ace0318a653ab6a1412fbe16684451890071fff5f15730e"},
    {"track.38.tile-flags", 211,
     "262dcaa402e5a24ccae6a4e6d54253a55799f1450de022810b314fc8794d29fb"},
    {"track.38.bg1-tiles", 27008,
     "50fc2d0eac5b043191c810124646a6d75c690d7344af3625aadc3a84cd9eaaad"},
    {"track.39.data", 51273, "95c3f44fc1c92869c9af12b1aa2b711c9bdab99452c35e384f8c23641c57f9cd"},
    {"track.39.tile-columns", 6272,
     "520b504f6f5c7f41b437b35eb27824973cce0e52677e56ce16bca7d14817ee0a"},
    {"track.39.tile-flags", 196,
     "d618ec25196316ed594260d55e2c76854e27833e8a5a39b92ed847cb350840f8"},
    {"track.39.bg1-tiles", 25088,
     "4a3e9cea90a55865b7f945cb7c862a838ceea4a0bfa25a52f1447fed470304d1"},
    {"track.40.data", 65354, "47ae70fab8eafcbce52de0776dc0c0805a09d253661a901ab823715f94e9b435"},
    {"track.40.tile-columns", 6528,
     "fb41b136b69898d1ad761a52fa4dcb0e4d4ddcd09dfb916e9769524edd5eb848"},
    {"track.40.tile-flags", 204,
     "d9518a088fbe3f203a3b7bb830600bc28b567797cb57220a384b912b1ef9b1d5"},
    {"track.40.bg1-tiles", 26112,
     "b5ddea1680adacdf86cb5b74b20df68e526d48bbe7e1010a2705a60ba2fec4c5"},
    {"track.41.data", 43463, "7b586e0badf8f2668ca480f1a238cb137a7b93a6b8cfb631083ea18fdf4b4626"},
    {"track.41.tile-columns", 4704,
     "533c4adc2f9c490009875235b73b8573f240d59af7578db46c5abc976628ea04"},
    {"track.41.tile-flags", 147,
     "ce9af875fd8b8f0ed10f44efc6646ef8e89b9c0c7b5c9f9d0a27f8465572c651"},
    {"track.41.bg1-tiles", 18816,
     "997c1767f5ef9775bd17cb76bbd3122c1b23030383026cb3ec18d3370b3bd729"},
    {"track.43.data", 49607, "2b58ac1c4a4cfd4920fbce8a7f63e86da3397619bf38ffc23e2fb9909659aec7"},
    {"track.43.tile-columns", 6432,
     "bd91ba2d25dd5876a4b5d3325dd2b9575ff1428005fda9cb3fa76f873ce49685"},
    {"track.43.tile-flags", 201,
     "86fcb15076fb4e04010d8c6fa60f67c2a455a7ed1129f570c919da0cc1b8835e"},
    {"track.43.bg1-tiles", 25728,
     "d06d0c554244b5b48fd2b33a95dc4c1df79f1f2db96bcdd8a4abf61800983910"},
    {"track.44.data", 44592, "10ec4e242dc63546fb5dd07b29401f3528ff9e8411775d85696cdba4995b71cf"},
    {"track.44.tile-columns", 7584,
     "0c3961d122c11480450ba3ea9eee5b07d1c0e0ee840afaa3e825a00f03a18a85"},
    {"track.44.tile-flags", 237,
     "3e18d8f508f007242baaf04f9e42f987d8d6fee274b1fd459113400146beaded"},
    {"track.44.bg1-tiles", 30336,
     "689ec66a8e941864f1533483ba4f7f5244ef6d5f178494ae270ef1b47f70ade8"},
    {"scenery.01.bg2-tiles", 672,
     "ebb6921ca08d48bd76c02879a92647521524eb87d4ab4d4209fd7add1f37d949"},
    {"scenery.01.bg2-map", 8192,
     "a47ec4ddf2128d1126c299a8dea5fde497c326b01d6bd0ccc97483bf0fb904a6"},
    {"scenery.01.palette", 352, "874033ba4d54a86a3706e7ba92a17019d6df2ea1a9a3178de2b675ed8f6a7de6"},
    {"scenery.08.bg2-tiles", 1056,
     "c1b3f928ea4204463b56128688a135e336777555933e111c8942acfe0130d9c2"},
    {"scenery.08.bg2-map", 8192,
     "a08bfaadfa15bfe42283809a829f8e764f5d29d26164aa43ffc79185abececb8"},
    {"scenery.08.palette", 352, "f55d4fe22370c5c3e72226ce44c3c2a273b69bd9a59d3b703fb521fb47be8e0c"},
    {"scenery.12.bg2-tiles", 736,
     "a544ae70df6b263760b3dbe29054414c47130233ff321fca46f1eca2647372eb"},
    {"scenery.12.bg2-map", 8192,
     "a9ad06f9426d4971d276a31bc87ca024a3c7ef21efee51037aafbd41a7a148c0"},
    {"scenery.12.palette", 352, "b2a9aefe13c1dc68454cf0a5c2bedb086c6e162dbea1df1f2ef108347d89ab63"},
}};
constexpr std::string_view two_track_rules_sha =
    "efbbf7467794e30c9c9931172a437e907ee041c29504e10882100a9d5e54aa38";

std::array<std::uint8_t, 32> hex_digest(std::string_view text) {
    if (text.size() != 64) throw std::logic_error("invalid compiled Classic SHA-256");
    const auto nibble = [](char value) -> std::uint8_t {
        if (value >= '0' && value <= '9') return static_cast<std::uint8_t>(value - '0');
        if (value >= 'a' && value <= 'f') return static_cast<std::uint8_t>(value - 'a' + 10);
        throw std::logic_error("invalid compiled Classic SHA-256 digit");
    };
    std::array<std::uint8_t, 32> out{};
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<std::uint8_t>((nibble(text[i * 2]) << 4U) | nibble(text[i * 2 + 1]));
    return out;
}

std::array<std::uint8_t, 32> sha256(std::span<const std::uint8_t> source) {
    constexpr std::array<std::uint32_t, 64> constants{
        {0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
         0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
         0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
         0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
         0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
         0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
         0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
         0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
         0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
         0xc67178f2U}};
    auto rotate = [](std::uint32_t value, unsigned bits) {
        return (value >> bits) | (value << (32U - bits));
    };
    std::vector<std::uint8_t> message(source.begin(), source.end());
    const auto bit_length = static_cast<std::uint64_t>(message.size()) * 8U;
    message.push_back(0x80);
    while (message.size() % 64U != 56U) message.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8)
        message.push_back(static_cast<std::uint8_t>(bit_length >> static_cast<unsigned>(shift)));
    std::array<std::uint32_t, 8> hash{{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                       0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U}};
    for (std::size_t base = 0; base < message.size(); base += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            const auto at = base + i * 4;
            words[i] = (static_cast<std::uint32_t>(message[at]) << 24U)
                     | (static_cast<std::uint32_t>(message[at + 1]) << 16U)
                     | (static_cast<std::uint32_t>(message[at + 2]) << 8U) | message[at + 3];
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 =
                rotate(words[i - 15], 7) ^ rotate(words[i - 15], 18) ^ (words[i - 15] >> 3U);
            const auto s1 =
                rotate(words[i - 2], 17) ^ rotate(words[i - 2], 19) ^ (words[i - 2] >> 10U);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        auto a = hash[0], b = hash[1], c = hash[2], d = hash[3], e = hash[4], f = hash[5],
             g = hash[6], h = hash[7];
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
            out[i * 4 + byte] = static_cast<std::uint8_t>(hash[i] >> (24U - byte * 8U));
    return out;
}
} // namespace

namespace {
constexpr std::array<std::string_view, 2> supported_profiles{"classic.pal.crawler.dragster.v1",
                                                             "classic.pal.crawler.tracks.v18"};
} // namespace

std::span<const std::string_view> supported_pack_profiles() {
    return supported_profiles;
}

namespace {

constexpr std::string_view pack_magic = "URCP0001";
constexpr std::uint32_t pack_schema = 1;
constexpr std::string_view pal_rom_sha256 =
    "a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e";
constexpr std::string_view dragster_rules_sha256 =
    "70712c470db436ad95b02d3a6d51f737be7bb5b27689ca0d99a8297bac31d768";
constexpr std::string_view tracks_start = "classic.crawler.race-start.v2";
constexpr std::string_view dragster_start = "classic.crawler.dragster.race-start.v1";

struct PackRow {
    std::string id;
    std::uint64_t offset, size;
    std::array<std::uint8_t, 32> digest;
};

// The header: magic, schema, source ROM, profile and start state, and the extraction
// rules' identity. Returns true for the tracks profile, false for DRAGSTER v1.
bool read_pack_header(const std::vector<std::uint8_t>& bytes, Reader& in) {
    if (bytes.size() < 12
        || std::string_view(reinterpret_cast<const char*>(bytes.data()), 8) != pack_magic)
        throw std::invalid_argument("Classic pack magic is unsupported");
    in.skip(8);
    if (in.u32() != pack_schema) throw std::invalid_argument("Classic pack schema is unsupported");
    const auto source_identity = in.digest();
    const auto rules_identity = in.digest();
    if (source_identity != hex_digest(pal_rom_sha256))
        throw std::invalid_argument("Classic pack source ROM identity is unsupported");
    const auto profile = in.text();
    const auto start = in.text();
    const bool tracks = profile == supported_profiles[1];
    if (profile != supported_profiles[tracks ? 1 : 0])
        throw std::invalid_argument("Classic pack profile is unsupported");
    if (start != (tracks ? tracks_start : dragster_start))
        throw std::invalid_argument("Classic pack start state is unsupported");
    if (rules_identity != hex_digest(tracks ? two_track_rules_sha : dragster_rules_sha256))
        throw std::invalid_argument("Classic pack extraction-rules identity is unsupported");
    return tracks;
}

// The entries a profile must carry, exactly: DRAGSTER v1's, and for the tracks profile the
// entries each later profile added.
std::vector<RequiredEntry> required_entries(bool tracks) {
    std::vector<RequiredEntry> out(required.begin(), required.end());
    if (!tracks) return out;
    for (const auto table : {std::span<const RequiredEntry>(zoom_required),
                             std::span<const RequiredEntry>(tracks_required),
                             std::span<const RequiredEntry>(special_tiles_required),
                             std::span<const RequiredEntry>(locked_tracks_required),
                             std::span<const RequiredEntry>(loop_required),
                             std::span<const RequiredEntry>(hunter_required),
                             std::span<const RequiredEntry>(front_end_required),
                             std::span<const RequiredEntry>(rider_menu_required),
                             std::span<const RequiredEntry>(one_player_screens_required),
                             std::span<const RequiredEntry>(race_result_required)})
        out.insert(out.end(), table.begin(), table.end());
    return out;
}

// The inventory: each entry's logical ID, payload offset, size and SHA-256, IDs unique.
std::vector<PackRow> read_inventory(Reader& in) {
    const auto count = in.u16();
    std::vector<PackRow> rows;
    rows.reserve(count);
    std::unordered_map<std::string, bool> seen;
    for (unsigned index = 0; index < count; ++index) {
        auto id = in.text();
        const auto offset = in.u64();
        const auto size = in.u64();
        const auto digest = in.digest();
        if (seen.contains(id))
            throw std::invalid_argument("Classic pack contains a duplicate logical ID");
        seen.emplace(id, true);
        rows.push_back({std::move(id), offset, size, digest});
    }
    return rows;
}

// The inventory holds exactly the required entries, each with its expected size and hash.
void check_inventory(const std::vector<PackRow>& rows, const std::vector<RequiredEntry>& expected) {
    if (rows.size() != expected.size())
        throw std::invalid_argument("Classic pack inventory is incomplete");
    for (const auto& want : expected) {
        const auto row = std::find_if(rows.begin(), rows.end(),
                                      [&](const PackRow& value) { return value.id == want.id; });
        if (row == rows.end())
            throw std::invalid_argument("Classic pack required logical entry is missing");
        if (row->size != want.size || row->digest != hex_digest(want.sha256))
            throw std::invalid_argument("Classic pack required entry identity is unsupported: "
                                        + std::string(want.id));
    }
}

} // namespace

// Reads and checks a pack: the header, the inventory against the profile's required entries,
// then each payload, which must follow the last without gaps and match its hash.
ClassicContentPack::ClassicContentPack(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open Classic content pack: " + path.string());
    bytes_ = {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    Reader in(bytes_);
    const bool tracks = read_pack_header(bytes_, in);
    const auto expected = required_entries(tracks);
    const auto rows = read_inventory(in);
    std::uint64_t cursor = in.offset();
    check_inventory(rows, expected);
    for (const auto& row : rows) {
        if (row.offset != cursor || row.size > std::numeric_limits<std::size_t>::max()
            || row.offset > bytes_.size()
            || row.size > bytes_.size() - static_cast<std::size_t>(row.offset))
            throw std::invalid_argument("Classic pack payload layout is invalid");
        const auto payload = std::span<const std::uint8_t>(bytes_).subspan(
            static_cast<std::size_t>(row.offset), static_cast<std::size_t>(row.size));
        if (sha256(payload) != row.digest)
            throw std::invalid_argument("Classic pack entry payload hash differs: " + row.id);
        entries_[row.id] = {static_cast<std::size_t>(row.offset),
                            static_cast<std::size_t>(row.size)};
        cursor += row.size;
    }
    if (cursor != bytes_.size()) throw std::invalid_argument("Classic pack has trailing data");
}

std::span<const std::uint8_t>
ClassicContentPack::optional_entry(const std::string& logical_id) const {
    return entries_.contains(logical_id) ? entry(logical_id) : std::span<const std::uint8_t>{};
}

std::span<const std::uint8_t> ClassicContentPack::entry(const std::string& logical_id) const {
    const auto found = entries_.find(logical_id);
    if (found == entries_.end())
        throw std::invalid_argument("Classic pack logical entry is absent: " + logical_id);
    return std::span<const std::uint8_t>(bytes_).subspan(found->second.offset, found->second.size);
}
} // namespace unirally
