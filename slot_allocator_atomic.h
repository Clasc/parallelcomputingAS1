#include <thread>
#include <cassert>
#include <atomic>
#include <iostream>
#include <mutex>

#if !defined(SLOT_ALLOCATOR_ATOMIC)
#define SLOT_ALLOCATOR_ATOMIC

#endif // SLOT_ALLOCATOR_ATOMIC

using namespace std;
struct slot
{
    int idx;
    slot *next;
    slot(int index) : idx{index} {};
};

struct slot_allocator_atomic
{
    slot_allocator_atomic()
    {
        slots_head = new slot(0);

        for (size_t i = 1; i < num_slots; i++)
        {
            auto new_slot = new slot(i);
            new_slot->next = slots_head;
            slots_head = new_slot;
        }
    }

    int acquire_slot()
    {
        slot *old_head = slots_head.load(memory_order::memory_order_acquire);

        while (!old_head || !slots_head.compare_exchange_weak(old_head, old_head->next, memory_order::memory_order_release))
        {
            cout << "failed acquiring slot for: " << this_thread::get_id();
        }

        return old_head->idx;
    }

    void release_slot(int slot_idx)
    {
        auto old_head = slots_head.load(memory_order::memory_order_acquire);
        slot *new_slot = new slot(slot_idx);
        new_slot->next = old_head;
        while (!old_head || !slots_head.compare_exchange_weak(old_head, new_slot, memory_order::memory_order_release))
        {
            cout << "failed releasing slot for: " << this_thread::get_id();
        }
    }

    void printSlots()
    {
        auto slot = slots_head.load(memory_order::memory_order_acquire);
        cout << "slots:{" << endl;
        while (slot)
        {
            cout << "   " << slot->idx << endl;
            slot = slot->next;
        }
        cout << "}" << endl;
    }

private:
    int num_slots = 8;
    atomic<slot *> slots_head;

    bool contains(int idx)
    {
        auto slot = slots_head.load();
        while (slot)
        {
            if (slot->idx == idx)
            {
                return true;
            }
            slot = slot->next;
        }
        return false;
    }
};