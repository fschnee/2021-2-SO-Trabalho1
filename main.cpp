#include <semaphore>
#include <csignal>
#include <cstdlib> // For std::exit;
#include <thread>
#include <vector>
#include <atomic>

#include "kol/utils/aliases.hpp"

#include "comms_system.hpp"
#include "guarded.hpp"
#include "rng.hpp"

using namespace kol::aliases;

struct meal
{
    u64 employee_id;
};

inline auto rand_ms(auto min, auto max)
{
    static auto rand = guarded< rng >{};
    return std::chrono::milliseconds{ rand.lock()->generate(min, max) };
}

// Needs to be here to be accessable from std::signal.
auto close_up_shop = std::atomic<bool>{ false };
auto close_up_shop_signal = std::binary_semaphore{0};

int main(int argc, const char* argv[])
{
    std::signal(SIGINT, []([[maybe_unused]] int signal)
    {
        close_up_shop = true;
        close_up_shop_signal.release();
    });
    std::signal(SIGABRT, []([[maybe_unused]] int signal) { std::exit(1); });

    auto employee_count = u64{4};

    auto parked_trunk        = guarded< std::vector<meal> >{};
    auto employees           = std::vector< std::jthread >{ employee_count };
    // Coordination stuff.
    auto do_delivery_signal  = std::binary_semaphore{0}; // Green when delivering.
    auto trunk_parked_signal = std::binary_semaphore{0}; // Green when the trunk is parked.
    // Flags for cooperatively stopping the program.
    auto open_restaurants = std::atomic<u64>{ employee_count };
    auto all_done_signal  = std::binary_semaphore{0};

    auto radio = comms_system{ employee_count };

    // Creating the employees.
    while(employee_count--) employees.push_back( std::jthread{ [&, id = employee_count] { while(true)
    {
        if(close_up_shop)
        {
            radio.notify( comms_system::closed_up_shop{id} );
            open_restaurants -= 1;
            return;
        }

        radio.notify( comms_system::started_preparing_meal{id} );
        std::this_thread::sleep_for(rand_ms(5000, 10000));
        radio.notify( comms_system::done_preparing_meal{id} );

        // And putting it in the trunk.
        auto pushed_meal = false;
        while(!pushed_meal)
        {
            parked_trunk.read([&](auto& trunk){
                if( trunk.size() == 10 ) { return; }

                radio.notify( comms_system::placing_meal_in_trunk{id} );
                std::this_thread::sleep_for(rand_ms(1000, 4000));
                trunk.push_back(meal{id});
                pushed_meal = true;

                if(trunk.size() == 10 || (close_up_shop && open_restaurants == 1))
                {
                    radio.notify( comms_system::waking_up_deliveryman{id} );
                    std::this_thread::sleep_for(rand_ms(1000, 2000));
                    do_delivery_signal.release();
                }
            });

            // Only happens when the trunk is parked and full.
            if(!pushed_meal) { trunk_parked_signal.acquire(); }
        }
    }}});

    auto deliveryman = std::jthread{ [&] { while(true)
    {
        // Sleep until woken up.
        do_delivery_signal.acquire();
        // No one can read <parked_trunk> until I release it.
        auto trunk = parked_trunk.lock();

        std::this_thread::sleep_for(rand_ms(1000, 5000)); // Im a bit sleepy still...
        radio.notify( comms_system::deliveryman_left{} );

        // Making the deliveries.
        for(auto remaining = trunk->size(); remaining > 0; --remaining)
        {
            std::this_thread::sleep_for(rand_ms(200, 2000));
            radio.notify( comms_system::meal_delivered{ trunk->back().employee_id } );
            trunk->pop_back();
        }

        radio.notify( comms_system::deliveryman_returning{} );
        std::this_thread::sleep_for(rand_ms(200, 500));
        radio.notify( comms_system::deliveryman_returned{} );

        trunk_parked_signal.release();

        if(open_restaurants == 0) { all_done_signal.release(); return; };
    }}};

    close_up_shop_signal.acquire();
    radio.notify( comms_system::close_up_shop{} );
    all_done_signal.acquire();

    return 0;
}
