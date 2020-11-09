#include <thread>
#include <cassert>
#include <atomic>
#include <iostream>
#include <vector>

#if !defined(SLOT_ALLOCATOR_ATOMIC_VECTOR)
#define SLOT_ALLOCATOR_ATOMIC_VECTOR

#endif // SLOT_ALLOCATOR_ATOMIC_VECTOR

using namespace std;

struct slot_allocator_atomic_vector
{
    slot_allocator_atomic_vector() : slots(num_slots, false) {}

    int acquire_slot()
    {

        int slot = 0;
        lock.test_and_set(memory_order::memory_order_acquire);
        for (;;)
        {
            for (auto i{0}; i < slots.size(); i++)
            {
                if (!slots[i])
                {
                    slots[i] = true;
                    lock.clear(memory_order::memory_order_release);
                    return i;
                }
            }
            lock.clear(memory_order::memory_order_release);
            this_thread::yield();
        }
    }

    void release_slot(int slot_idx)
    {
        lock.test_and_set(memory_order::memory_order_acquire);
        slots[slot_idx] = false;
        lock.clear(memory_order::memory_order_release);
    }

private:
    int num_slots = 10;
    vector<bool> slots;
    atomic_flag lock;
};