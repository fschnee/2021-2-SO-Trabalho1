// Who likes long types?
#pragma once

namespace kol::inline aliases
{
    namespace detail
    {
        template <auto...>
        struct false_type { static constexpr bool value = false; };

        template <int Size>
        constexpr auto integer_sized_impl()
        {
            constexpr auto size = Size/8;
            if      constexpr(sizeof(char)      == size) { return static_cast<char>(0); }
            else if constexpr(sizeof(short)     == size) { return static_cast<short>(0); }
            else if constexpr(sizeof(int)       == size) { return static_cast<int>(0); }
            else if constexpr(sizeof(long)      == size) { return static_cast<long>(0); }
            else if constexpr(sizeof(long long) == size) { return static_cast<long long>(0); }
            else { static_assert(false_type<Size>::value, "No type of this size"); }
        }

        #define KOL_INTALIAS_MAKE_SIGNED_IMPL(cls, signedness)                         \
            template <typename T> struct cls;                                          \
            template <> struct cls <char>      { using type = signedness char; };      \
            template <> struct cls <short>     { using type = signedness short; };     \
            template <> struct cls <int>       { using type = signedness int; };       \
            template <> struct cls <long>      { using type = signedness long; };      \
            template <> struct cls <long long> { using type = signedness long long; }; \

        KOL_INTALIAS_MAKE_SIGNED_IMPL(u, unsigned)
        KOL_INTALIAS_MAKE_SIGNED_IMPL(s, signed)
    }

    template <int Size> using uint = typename detail::u< decltype(detail::integer_sized_impl<Size>()) >::type;
    template <int Size> using sint = typename detail::s< decltype(detail::integer_sized_impl<Size>()) >::type;

    using u8  = uint<8>;
    using u16 = uint<16>;
    using u32 = uint<32>;
    using u64 = uint<64>;

    using i8  = sint<8>;
    using i16 = sint<16>;
    using i32 = sint<32>;
    using i64 = sint<64>;

    inline namespace literals
    {
        constexpr auto operator""_u8(unsigned long long val)  { return static_cast<u8>(val); }
        constexpr auto operator""_u16(unsigned long long val) { return static_cast<u16>(val); }
        constexpr auto operator""_u32(unsigned long long val) { return static_cast<u32>(val); }
        constexpr auto operator""_u64(unsigned long long val) { return static_cast<u64>(val); }

        constexpr auto operator""_i8(unsigned long long val)  { return static_cast<i8>(val); }
        constexpr auto operator""_i16(unsigned long long val) { return static_cast<i16>(val); }
        constexpr auto operator""_i32(unsigned long long val) { return static_cast<i32>(val); }
        constexpr auto operator""_i64(unsigned long long val) { return static_cast<i64>(val); }
    };
}
