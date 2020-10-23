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
        setDeletable(false);

        for (size_t i = 1; i < num_slots; i++)
        {
            auto new_slot = new slot(i);
            new_slot->next = slots_head;
            slots_head = new_slot;
        }
    }

    int acquire_slot()
    {
        setDeletable(false);

        slot *old_head = slots_head.load(memory_order::memory_order_acquire);

        while (!old_head || !slots_head.compare_exchange_weak(old_head, old_head->next, memory_order::memory_order_release))
        {
            //cout << "failed acquiring slot for: " << this_thread::get_id() << endl;
            old_head = slots_head.load(memory_order::memory_order_acquire);
        }

        auto idx = old_head->idx;
        setDeletable(true);

        while (!isDeletable())
        {
            cout << "Wait for delete" << endl;
        }

        delete old_head;

        return idx;
    }

    void release_slot(int slot_idx)
    {
        setDeletable(false);

        auto old_head = slots_head.load(memory_order::memory_order_acquire);
        slot *new_slot = new slot(slot_idx);

        if (old_head)
        {
            new_slot->next = old_head;
        }

        while (!slots_head.compare_exchange_weak(old_head, new_slot, memory_order::memory_order_release))
        {
            //cout << "failed releasing slot for: " << this_thread::get_id() << endl;
            old_head = slots_head.load(memory_order::memory_order_acquire);
            if (old_head)
            {
                new_slot->next = old_head;
            }
        }

        setDeletable(true);

        if (old_head == nullptr)
        {
            return;
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
    int num_slots = 2;
    atomic<slot *> slots_head;
    atomic<bool> deletable;

    bool isDeletable()
    {
        return deletable.load(memory_order::memory_order_acquire);
    }

    void setDeletable(bool del)
    {
        return deletable.store(del, memory_order::memory_order_release);
    }
};