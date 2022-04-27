#pragma once

#include <memory>
#include <mutex>

template <class T>
struct guarded
{
    constexpr guarded(guarded&&) = delete;
    constexpr guarded(const guarded&) = delete;
    constexpr auto operator=(guarded&&) -> guarded& = delete;
    constexpr auto operator=(const guarded&) -> guarded& = delete;

    constexpr guarded() : mut{}, data{} {}
    constexpr guarded(auto&&... args)
        : mut{}
        , data{ std::forward< decltype(args) >(args)... }
    {}

    inline auto read(auto&& func)
    {
        auto lock = std::lock_guard(mut);
        return func(data);
    }

    constexpr auto lock()
    {
        struct unlocker
        {
            guarded& g;
            constexpr auto operator()(T*){ g.mut.unlock(); }
        };

        mut.lock();
        return std::unique_ptr<T, unlocker>(&data, {*this});
    }

private:
    std::mutex mut;
    T data;
};
