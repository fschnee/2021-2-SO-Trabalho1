#pragma once

#include "kol/utils/aliases.hpp"

#include <string_view>

namespace kol
{
    template <class T> struct remove_ref      { using type = T; };
    template <class T> struct remove_ref<T&>  { using type = T; };
    template <class T> struct remove_ref<T&&> { using type = T; };

    template <bool B, class T = void> struct enable_if {};
    template <class T> struct enable_if<true, T> { using type = T; };
    template <bool B, class T = void> using enable_if_t = typename enable_if<B,T>::type;

    #define KOL_FWD(x) static_cast<decltype(x)&&>(x)
    #define KOL_MOV(x) static_cast<typename kol::remove_ref<decltype(x)>::type&& >(x)

    constexpr auto swap(auto& a, auto& b)
    {
        auto c = KOL_MOV(a);
        a = KOL_MOV(b);
        b = KOL_MOV(c);
    }

    constexpr auto max(auto arg1, auto arg2, auto... args)
    {
        if constexpr (sizeof...(args) > 0) return arg1 > arg2 ? max(arg1, args...) : max(arg2, args...);
        else                               return arg1 > arg2 ? arg1               : arg2;
    }

    template <auto v> struct val { static constexpr auto value = v; };

    // Stuff for dealing with type lists.
    namespace tl
    {
        template <class, class> struct index_of;
        // Base cases.
        template <class T, template <class...> class L>              struct index_of<T, L<>>         : val<0_u64> {};
        template <class T, template <class...> class L, class... Ts> struct index_of<T, L<T, Ts...>> : val<0_u64> {};
        // Recursive case.
        template <class T, class Other, template <class...> class L, class... Ts>
        struct index_of<T, L<Other, Ts...>> : val< 1 + index_of<T, L<Ts...>>::value > {};

        template <class, class> struct contains;
        template <class T, template <class...> class L, class... Ts>
        struct contains<T, L<Ts...>> : val< index_of<T, L<Ts...>>::value != sizeof...(Ts) > {};

        struct not_found;

        template <auto, class> struct indexed;
        // Base cases.
        template <template <class...> class L, class T, class... Ts>
        struct indexed<0_u64, L<T, Ts...>> : val<true> { using type = T; };
        template <u64 I, template <class...> class L>
        struct indexed<I, L<>> : val<false> { using type = not_found; };
        // Recursive case.
        template <u64 I, template <class...> class L, class T, class... Ts>
        struct indexed<I, L<T, Ts...>> : public indexed<I - 1, L<Ts...>> {};

        template <class, class, class> struct drop;
        // Base case.
        template <class T, template <class...> class Remaining, class Acc>
        struct drop<T, Remaining<>, Acc> { using type = Acc; };
        // Recursive cases.
        template <
            class T,
            template <class...> class Remaining, class T2, class... Ts,
            template <class...> class Acc,
            class... AccTs
        >
        struct drop< T, Remaining<T2, Ts...>, Acc<AccTs...> >
            :  drop< T, Remaining<Ts...>, Acc<AccTs..., T2> >
        {};
        template <
            class T,
            template <class...> class Remaining, class... Ts,
            template <class...> class Acc,
            class... AccTs
        >
        struct drop< T, Remaining<T, Ts...>, Acc<AccTs...> >
            :  drop< T, Remaining<Ts...>, Acc<AccTs...> >
        {};
    }
}
