// I don't like std::variant, too verbose and heavy.
#pragma once

#include "kol/utils/utility.hpp"

#include <cstddef> // For std::byte

namespace kol { template <class...> struct variant; }

// Can only handle variants with more than 1 type.
// Advertises copyability to work with incomplete types.
template <class... Ts>
struct kol::variant
{
    constexpr auto is_valid() const { return index != sizeof...(Ts); }
    template <class T>
    constexpr auto holds()    const { return tl::index_of<T, variant>::value == index; }
    template <u64 I>
    constexpr auto holds() const
    requires tl::indexed<I, variant>::value
    { return holds< typename tl::indexed<I, variant>::type >(); }

    template <class T, class Functor>
    constexpr auto on(Functor&& f) const -> variant const&
    {
        if(holds<T>()) f( as<T>() );
        return *this;
    }

    template <class T, class Functor>
    constexpr auto on(Functor&& f) -> variant&
    {
        if(holds<T>()) f( as<T>() );
        return *this;
    }

    constexpr auto invalidate()
    {
        destroy(*this);
        index   = sizeof...(Ts);
        copy    = [](auto&, auto&){};
        move    = [](auto&, auto&&){};
        destroy = [](auto&){};
    }

    template <class T> constexpr auto as() &      -> T&       { return *reinterpret_cast<T*>(&data); }
    template <class T> constexpr auto as() const& -> T const& { return *reinterpret_cast<T const*>(&data); }
    template <class T> constexpr auto as() &&     -> T&&      { return KOL_MOV(*reinterpret_cast<T*>(&data)); }

    template <u64 I>
    constexpr auto as() & -> typename tl::indexed<I, variant>::type&
    requires tl::indexed<I, variant>::value
    { return as< typename tl::indexed<I, variant>::type >(); }

    template <u64 I>
    constexpr auto as() const& -> typename tl::indexed<I, variant>::type const&
    requires tl::indexed<I, variant>::value
    { return as< typename tl::indexed<I, variant>::type >(); }

    template <u64 I>
    constexpr auto as() && -> typename tl::indexed<I, variant>::type&&
    requires tl::indexed<I, variant>::value
    { return static_cast<typename tl::indexed<I, variant>::type&&>( as< typename tl::indexed<I, variant>::type >() ); }

    template <class T>
    constexpr auto drop() &&
    {
        using ret_t = typename tl::drop<T, variant, variant<>>::type;

        auto ret = ret_t{};

        (on<Ts>([&](auto& inner){
            if constexpr(tl::contains<Ts, ret_t>::value) ret = KOL_MOV(inner);
        }), ...);

        return ret;
    }

    template <class T>
    constexpr auto dropped() const&
    {
        using ret_t = typename tl::drop<T, variant, variant<>>::type;

        auto ret = ret_t{};

        (on<Ts>([&](auto& inner){
            if constexpr(tl::contains<Ts, ret_t>::value) ret = inner;
        }), ...);

        return ret;
    }

    constexpr variant() = default;

    template <typename T>
    constexpr variant(T&& t)
    requires tl::contains<T, variant>::value
    {
        new (&data) T(KOL_FWD(t));
        index   = tl::index_of<T, variant>::value;
        copy    = [](auto& src, auto& tgt) { new (&tgt.data) T(src.template as<T>()); };
        move    = [](auto& tgt, auto&& src) { new (&tgt.data) T(KOL_MOV(src).template as<T>()); };
        destroy = [](auto& v) { v.template as<T>().~T(); };
    }

    constexpr variant(const variant& other)     { *this = other; }
    constexpr variant(variant&& other) noexcept { *this = KOL_MOV(other); }

    template <class T>
    constexpr auto operator=(T&& t) -> variant&
    requires tl::contains<T, variant>::value
    {
        destroy(*this);

        new (&data) T(KOL_FWD(t));
        index   = tl::index_of<T, variant>::value;
        copy    = [](auto& src, auto& tgt) { new (&tgt.data) T(src.template as<T>()); };
        move    = [](auto& tgt, auto&& src) { new (&tgt.data) T(KOL_MOV(src).template as<T>()); };
        destroy = [](auto& v) { v.template as<T>().~T(); };

        return *this;
    }

    constexpr auto operator=(const variant& other) -> variant&
    {
        destroy(*this);
        other.copy(other, *this);

        index   = other.index;
        copy    = other.copy;
        move    = other.move;

        destroy = other.destroy;

        return *this;
    }

    constexpr auto operator=(variant&& other) noexcept -> variant&
    {
        destroy(*this);
        other.move(*this, KOL_MOV(other));

        index = other.index;
        copy  = other.copy;
        move  = other.move;
        destroy = other.destroy;
        other.invalidate();

        return *this;
    }

    constexpr ~variant() { destroy(*this); }

private:
    unsigned long long index = sizeof...(Ts);

    alignas(Ts...) std::byte data[max(sizeof(Ts)...)];

    using copy_t    = void(*)(variant const&, variant&);
    using move_t    = void(*)(variant&, variant&&);
    using destroy_t = void(*)(variant&);

    copy_t copy       = [](auto&, auto&){};
    move_t move       = [](auto&, auto&&){};
    destroy_t destroy = [](auto&){};
};
