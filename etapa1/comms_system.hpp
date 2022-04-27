#pragma once

#include "kol/utils/variant.hpp"
#include "kol/utils/aliases.hpp"

#include "guarded.hpp"

#include <semaphore>
#include <thread>
#include <queue>

struct comms_system
{
    // Notifications.
    struct started_preparing_meal { kol::u64 employee_id; };
    struct done_preparing_meal { kol::u64 employee_id; };
    struct placing_meal_in_trunk { kol::u64 employee_id; };
    struct waking_up_deliveryman { kol::u64 employee_id; };
    struct deliveryman_left {};
    struct meal_delivered { kol::u64 employee_id; };
    struct deliveryman_returning {};
    struct deliveryman_returned {};
    struct close_up_shop {};
    struct closed_up_shop { kol::u64 employee_id; };

    using notification = kol::variant<
        started_preparing_meal,
        done_preparing_meal,
        placing_meal_in_trunk,
        waking_up_deliveryman,
        deliveryman_left,
        meal_delivered,
        deliveryman_returning,
        deliveryman_returned,
        close_up_shop,
        closed_up_shop
    >;

    constexpr comms_system(auto ec)
        : done{ false }
        , notifications{ }
        , ready_output_signal{ 0 }
        , output_thread{ [&]{ while(!(*done.lock())) flush_output(); } }
        , employee_count{ec}
    {
        std::printf("-- Press Ctrl+C to close up shop (will need to wait for all the deliveries to be dispatched)\n\n");

        std::printf("STATUS: [%lu] Restaurants are open\n", deliveries_done);
        for(auto i = kol::u64{0}; i < employee_count + 1; ++i) std::printf("\n");

        go_up_lines(employee_count);
        std::printf("DELIVERYMAN: [%lu/10] Zzz...", meals_in_trunk);
        go_down_lines(employee_count);
        std::fflush(stdout);
    }

    inline auto notify(notification&& _notification)
    {
        notifications.read([&](auto& ns){ ns.push( std::move(_notification) ); });
        ready_output_signal.release();
    }

    inline auto flush_output() -> void
    {
        while(output()) {;}
        ready_output_signal.acquire();
    }

    // Flush and stop the thread.
    ~comms_system()
    {
        ready_output_signal.release();
        *done.lock() = true;
        ready_output_signal.release();
    }

private:
    inline auto go_up_lines(kol::u64 count) -> void
    {
        while (1 + count--) std::printf("\033[F");
        std::printf("\033[K"); // Erase current line.
    }

    inline auto go_down_lines(kol::u64 count) -> void
    { while(1 + count--) std::printf("\033[E"); }

    inline auto printf_on_line(kol::u64 line, const char* fmt, auto&&... args)
    {
        go_up_lines(line);
        std::printf(fmt, std::forward< decltype(args) >(args)...);
        go_down_lines(line);
    }

    inline auto output() -> bool
    {
        auto n = notification{};
        {
            auto ns = notifications.lock();
            // Early exit when there is nothing to print.
            if(ns->size() == 0) return false;

            n = std::move(ns->front());
            ns->pop();
        }

        n.on<started_preparing_meal>([&](auto& n)
        { printf_on_line(n.employee_id, "%lu: Preparing meal", n.employee_id); }
        ).on<done_preparing_meal>([&](auto& n)
        { printf_on_line(n.employee_id, "%lu: Preparation done, waiting to place meal", n.employee_id); }
        ).on<placing_meal_in_trunk>([&](auto& n)
        {
            ++meals_in_trunk;

            printf_on_line(n.employee_id, "%lu: Placing meal in the trunk", n.employee_id);
            printf_on_line(employee_count, "DELIVERYMAN: [%lu/10] Zzz...", meals_in_trunk);
        }
        ).on<waking_up_deliveryman>([&](auto& n)
        {
            printf_on_line(n.employee_id, "%lu: Waking up deliveryman", n.employee_id);
            printf_on_line(employee_count, "DELIVERYMAN: [%lu/10] Waking up", meals_in_trunk);
        }
        ).on<deliveryman_left>([&](auto&)
        { printf_on_line(employee_count, "DELIVERYMAN: [%lu/10] Started delivery", meals_in_trunk); }
        ).on<meal_delivered>([&](auto& n)
        {
            --meals_in_trunk;
            ++deliveries_done;

            printf_on_line(
                employee_count + 1,
                (closing_up_shop ? "STATUS: [%lu] Closing up shop!" : "STATUS: [%lu] Restaurants are open"),
                deliveries_done
            );

            printf_on_line(employee_count, "DELIVERYMAN: [%lu/10] Delivered meal from %lu", meals_in_trunk, n.employee_id);
        }
        ).on<deliveryman_returning>([&](auto&)
        { printf_on_line(employee_count, "DELIVERYMAN: [%lu/10] Returning", meals_in_trunk); }
        ).on<deliveryman_returned>([&](auto&)
        { printf_on_line(employee_count, "DELIVERYMAN: [%lu/10] Zzz...", meals_in_trunk); }
        ).on<close_up_shop>([&](auto&)
        {
            closing_up_shop = true;
            printf_on_line(employee_count + 1, "STATUS: [%lu] Closing up shop!", deliveries_done);
        }
        ).on<closed_up_shop>([&](auto& n)
        { printf_on_line(n.employee_id, "%lu: Closed up shop\n", n.employee_id); });

        std::fflush(stdout);

        return true;
    }

    // Shared with the thread.
    guarded< bool > done;
    guarded< std::queue<notification> > notifications;
    std::binary_semaphore ready_output_signal;

    // Thread-local.
    std::jthread output_thread;
    kol::u64 employee_count;
    kol::u64 meals_in_trunk = 0;
    kol::u64 deliveries_done = 0;
    bool closing_up_shop = false;
};
