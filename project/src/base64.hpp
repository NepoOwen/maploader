// File: base64.hpp
// Author: NepoOwen
// https://github.com/NepoOwen/cpp-base64
// Description: Base64 encoding and decoding functions for C++20. This implementation is designed to be fast and efficient, using a lookup table for decoding and a simple algorithm for encoding. It supports standard Base64 characters and handles padding correctly.
// Usage: base64::encode("Hello, World!") or base64::decode("SGVsbG8sIFdvcmxkIQ==")

#pragma once
#include <string>
#include <cstdint>

namespace base64 {

    namespace detail {

        static constexpr uint8_t kDecodeTable[256] = {
        //  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x00
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x10
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, // 0x20  (+, /)
            0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, // 0x30  (0-9, =)
            0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, // 0x40  (A-O)
            0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x50  (P-Z)
            0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, // 0x60  (a-o)
            0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0x70  (p-z)
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        };

        static inline bool decode_block4(const char* src, uint8_t* dst) noexcept {
            const uint8_t a = kDecodeTable[static_cast<uint8_t>(src[0])];
            const uint8_t b = kDecodeTable[static_cast<uint8_t>(src[1])];
            const uint8_t c = kDecodeTable[static_cast<uint8_t>(src[2])];
            const uint8_t d = kDecodeTable[static_cast<uint8_t>(src[3])];
            if ((a | b | c | d) >= 0xFE) return false;

            const uint32_t n = (uint32_t(a) << 18) | (uint32_t(b) << 12)
                | (uint32_t(c) << 6) | uint32_t(d);
            dst[0] = static_cast<uint8_t>(n >> 16);
            dst[1] = static_cast<uint8_t>(n >> 8);
            dst[2] = static_cast<uint8_t>(n);
            return true;
        }

    } // namespace detail

    inline std::string decode(const std::string& input) {
        const char* src = input.data();
        const size_t len = input.size();

        std::string result;
        result.resize((len / 4) * 3 + 3);
        auto* dst = reinterpret_cast<uint8_t*>(result.data());
        size_t out = 0;

        size_t i = 0;
        for (; i + 4 <= len; i += 4) {
            if (!detail::decode_block4(src + i, dst + out)) {
                break;
            }
            out += 3;
        }

        {
            int val = 0;
            int bits = -8;
            for (size_t j = i; j < len; ++j) {
                const uint8_t d = detail::kDecodeTable[static_cast<uint8_t>(src[j])];
                if (d == 0xFE) break;   // '=' - stop
                if (d == 0xFF) continue; // whitespace / invalid - skip
                val = (val << 6) | d;
                bits += 6;
                if (bits >= 0) {
                    dst[out++] = static_cast<uint8_t>((val >> bits) & 0xFF);
                    bits -= 8;
                }
            }
        }

        result.resize(out);
        return result;
    }

    inline std::string encode(const std::string& input) {
        static constexpr char kTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        const auto* src = reinterpret_cast<const uint8_t*>(input.data());
        const size_t len = input.size();

        std::string out;
        out.resize(((len + 2) / 3) * 4);
        char* dst = out.data();

        size_t i = 0, o = 0;
        for (; i + 3 <= len; i += 3, o += 4) {
            const uint32_t n = (uint32_t(src[i]) << 16)
                | (uint32_t(src[i + 1]) << 8)
                | uint32_t(src[i + 2]);
            dst[o + 0] = kTable[(n >> 18) & 0x3F];
            dst[o + 1] = kTable[(n >> 12) & 0x3F];
            dst[o + 2] = kTable[(n >> 6) & 0x3F];
            dst[o + 3] = kTable[(n >> 0) & 0x3F];
        }
        if (i < len) {
            uint32_t n = uint32_t(src[i]) << 16;
            if (i + 1 < len) n |= uint32_t(src[i + 1]) << 8;
            dst[o + 0] = kTable[(n >> 18) & 0x3F];
            dst[o + 1] = kTable[(n >> 12) & 0x3F];
            dst[o + 2] = (i + 1 < len) ? kTable[(n >> 6) & 0x3F] : '=';
            dst[o + 3] = '=';
        }
        return out;
    }

} // namespace base64
