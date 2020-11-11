#pragma once

#include <stdint.h>
#include <type_traits>

class Endian {
private:
    static constexpr uint16_t number = 0xAABB;
    static constexpr uint8_t view = (const uint8_t&)number;
public:
    Endian() = delete;
    static constexpr bool Little = view == 0xBB;
    static constexpr bool Big = view == 0xAA;
    static constexpr bool IsLittleEndian() { return view == 0xBB; }
    static constexpr bool IsBigEndian() { return view == 0xAA; }
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
