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
    slot(int index) : idx{index}, next{nullptr} {};
    slot(int index, slot *next) : idx{index}, next{next} {};

    ~slot()
    {
        if (!next)
        {
            return;
        }

        delete next;
    }
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

    ~slot_allocator_atomic()
    {
        auto slot = slots_head.load();
        auto deletable_head = deletable_slots.load();
        if (slot)
        {
            delete slot;
        }

        if (deletable_head)
        {
            // delete deletable_head;
        }
    }

    int acquire_slot()
    {
        slot *old_head = slots_head.load(memory_order::memory_order_acquire);

        while (!old_head || !slots_head.compare_exchange_weak(old_head, old_head->next, memory_order::memory_order_release))
        {
            //cout << "failed acquiring slot for: " << this_thread::get_id() << endl;
            old_head = slots_head.load(memory_order::memory_order_acquire);
        }

        set_deletable(old_head);
        return old_head->idx;
    }

    void release_slot(int slot_idx)
    {
        auto old_head = slots_head.load(memory_order::memory_order_acquire);
        slot *new_slot = new slot(slot_idx, old_head);

        while (!slots_head.compare_exchange_weak(old_head, new_slot, memory_order::memory_order_release))
        {
            //cout << "failed releasing slot for: " << this_thread::get_id() << endl;
            old_head = slots_head.load(memory_order::memory_order_acquire);
            if (old_head)
            {
                new_slot->next = old_head;
            }
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
    int num_slots = 10;
    atomic<slot *> slots_head;
    atomic<slot *> deletable_slots;

    void set_deletable(slot *slot)
    {
        auto old_head = deletable_slots.load(memory_order::memory_order_acquire);
        slot->next = old_head;
        while (!deletable_slots.compare_exchange_weak(old_head, slot, memory_order::memory_order_release))
        {
            old_head = deletable_slots.load(memory_order::memory_order_acquire);
            slot->next = old_head;
        }
    }
};