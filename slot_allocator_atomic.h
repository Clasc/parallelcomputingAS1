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
        slot *old_head = slots_head.load();

        waitForValidHead(old_head);

        slot *new_head = old_head->next;
        while (!slots_head.compare_exchange_weak(old_head, new_head))
        {
            old_head = slots_head.load();
        }

        return old_head->idx;
    }

    void release_slot(int slot_idx)
    {
        slot *new_slot = new slot(slot_idx);
        auto old_head = slots_head.load();
        new_slot->next = old_head;
        while (!slots_head.compare_exchange_weak(old_head, new_slot))
        {
            new_slot->next = old_head;
        }
    }

    void printSlots()
    {
        auto slot = slots_head.load();
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

    void waitForValidHead(slot *old_head)
    {
        while (!old_head)
        {
            this_thread::yield();
            old_head = slots_head.load();
        }
    }
};