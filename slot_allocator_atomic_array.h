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
                auto is_locked = slots[i].load(memory_order::memory_order_acquire);
                if (is_locked)
                {
                    continue;
                }

                while (!slots[i].compare_exchange_weak(is_locked, true, memory_order::memory_order_release))
                {
                    is_locked = slots[i].load(memory_order::memory_order_acquire);
                }
                return i;
            }
            this_thread::yield();
        }
    }

    void release_slot(int slot_idx)
    {
        bool is_locked = slots[slot_idx].load();
        while (!slots[slot_idx].compare_exchange_weak(is_locked, false))
        {
            is_locked = slots[slot_idx].load();
            this_thread::yield();
        }
    }

private:
    int num_slots = 10;
    atomic<bool> *slots;
};