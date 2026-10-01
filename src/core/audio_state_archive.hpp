#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace unirally::audio_state_detail {
// Fixed little-endian scalars and binary flags; never native struct padding.
class Archive {
public:
    Archive() = default;
    explicit Archive(std::span<const std::uint8_t> source) : source_(source), loading_(true) {}
    template <class... Values>
    void fields(Values&... values) {
        (value(values), ...);
    }
    template <class Value>
    void value(Value& item) {
        if constexpr (std::is_enum_v<Value>) {
            auto scalar = static_cast<std::underlying_type_t<Value>>(item);
            value(scalar);
            item = static_cast<Value>(scalar);
        } else if constexpr (std::is_same_v<Value, bool>) {
            std::uint8_t scalar = item ? 1 : 0;
            value(scalar);
            if (scalar > 1) throw std::invalid_argument("audio state flag is not binary");
            item = scalar != 0;
        } else if constexpr (std::is_integral_v<Value>) {
            using Unsigned = std::make_unsigned_t<Value>;
            Unsigned scalar = static_cast<Unsigned>(item);
            if (loading_) {
                scalar = 0;
                for (unsigned i = 0; i < sizeof(Value); ++i) {
                    if (offset_ == source_.size())
                        throw std::invalid_argument("audio state truncated");
                    scalar = static_cast<Unsigned>(
                        scalar | (std::uint64_t(source_[offset_++]) << (i * 8)));
                }
                if constexpr (std::is_signed_v<Value>)
                    item = std::bit_cast<Value>(scalar);
                else
                    item = scalar;
            } else {
                for (unsigned i = 0; i < sizeof(Value); ++i)
                    output_.push_back(static_cast<std::uint8_t>(scalar >> (i * 8)));
            }
        } else {
            visit(*this, item);
        }
    }
    template <class Value, std::size_t Size>
    void value(std::array<Value, Size>& items) {
        for (auto& item : items) value(item);
    }
    template <class Value>
    void value(std::vector<Value>& items) {
        constexpr std::uint32_t maximum_vector_bytes = 32 * 1024 * 1024;
        if (items.size() > maximum_vector_bytes / sizeof(Value))
            throw std::invalid_argument("audio state vector too large");
        auto count = static_cast<std::uint32_t>(items.size());
        value(count);
        if (count > maximum_vector_bytes / sizeof(Value))
            throw std::invalid_argument("audio state vector count too large");
        if (loading_) {
            if (count > source_.size() - offset_)
                throw std::invalid_argument("audio state vector truncated");
            items.resize(count);
        }
        for (auto& item : items) value(item);
    }
    void index(std::size_t& item) {
        auto scalar = static_cast<std::uint64_t>(item);
        value(scalar);
        if (scalar > std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("audio state index is not representable");
        item = static_cast<std::size_t>(scalar);
    }
    void require_end() const {
        if (offset_ != source_.size())
            throw std::invalid_argument("audio state has trailing bytes");
    }
    std::vector<std::uint8_t> take_output() { return std::move(output_); }

private:
    std::span<const std::uint8_t> source_;
    std::size_t offset_ = 0;
    bool loading_ = false;
    std::vector<std::uint8_t> output_;
};
} // namespace unirally::audio_state_detail
