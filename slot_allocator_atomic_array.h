#include <thread>
#include <cassert>
#include <atomic>
#include <iostream>
#include <vector>

#if !defined(SLOT_ALLOCATOR_ATOMIC_ARRAY)
#define SLOT_ALLOCATOR_ATOMIC_ARRAY

#endif // SLOT_ALLOCATOR_ATOMIC_ARRAY

using namespace std;
struct slot_allocator_atomic_array
{
    slot_allocator_atomic_array()
    {
        slots = new atomic<bool>[num_slots];
    }

    ~slot_allocator_atomic_array()
    {
        delete[] slots;
    }

    int acquire_slot()
    {
        for (;;)
        {
            for (auto i{0}; i < num_slots; i++)
            {
                if (slots[i].load())
                {
                    continue;
                }

                auto is_locked = false;
                while (!slots[i].compare_exchange_strong(is_locked, true, memory_order::memory_order_acquire))
                {
                    is_locked = false;
                };

                return i;
            }
        }
    }

    void release_slot(int slot_idx)
    {
        bool is_locked = slots[slot_idx].load();
        assert(is_locked);
        slots[slot_idx].store(false, memory_order::memory_order_release);
    }

private:
    int num_slots = 10;
    atomic<bool> *slots;
    /**
     * Performance:
     * Depending on the system this is compiled and run on.
     * On my machine (Macbook pro 2016) -
     * - Just Mutexes take around ~0.035 sec.
     * - Atomic Array take around ~0.07 sec.
     * 
     * On ALMA we are getting a different result.
     * - Just Mutexes take around 0.3 sec.
     * - Atomic Array takes around 0.1 sec.
     */
};
