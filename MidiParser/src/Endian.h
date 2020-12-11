#pragma once

#include <stdint.h>
#include <type_traits>

class Endian {
private:
    static constexpr uint16_t m_Number = 0xAABB;
    static constexpr uint8_t m_View = (const uint8_t&)m_Number;
public:
    Endian() = delete;

    static constexpr bool Little = (m_View == 0xBB);  // True if computer is little endian
    static constexpr bool Big = (m_View == 0xAA);  // True if computer is big endian

    static_assert(Little || Big, "Cannot determine endianness");

    template <typename T>
    static T FlipEndian(T number) {
        static_assert(std::is_integral<T>(), "Type T is not an integer");

        if constexpr (sizeof(T) > 1) {
            T destination = 0;
            uint8_t* source = reinterpret_cast<uint8_t*>(&number);

            for (int i = 0; i < sizeof(T); i++)
                destination += source[sizeof(T) - 1 - i] << (i * 8);

            return destination;
        } else {
            return number;
        }
    }
};
