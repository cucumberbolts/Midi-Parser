#pragma once

#include <cstdint>

class Endian {
private:
    static constexpr uint16_t m_Number = 0xAABB;
    static constexpr uint8_t m_View = (const uint8_t&)m_Number;
public:
    Endian() = delete;

    static constexpr bool Little = (m_View == 0xBB);  // True if computer is little endian
    static constexpr bool Big = (m_View == 0xAA);  // True if computer is big endian

    static_assert(Little || Big, "Cannot determine endianness");

    static inline uint16_t FlipEndian(uint16_t number) {
        uint8_t* source = reinterpret_cast<uint8_t*>(&number);
        return source[1] | source[0] << 8;
    }

    static inline uint32_t FlipEndian(uint32_t number) {
        uint8_t* source = reinterpret_cast<uint8_t*>(&number);
        return source[3] | source[2] << 8 | source[1] << 16 | source[0] << 24;
    }
};
