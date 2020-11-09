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
        if (next == NULL)
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
        // setDeletable(false);

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
        if (slot == NULL)
        {
            return;
        }

        delete slot;
    }

    int acquire_slot()
    {
        slot *old_head = slots_head.load(memory_order::memory_order_acquire);

        exec_delete_safe([this, &old_head]() {
            while (!old_head || !slots_head.compare_exchange_weak(old_head, old_head->next, memory_order::memory_order_release))
            {
                //cout << "failed acquiring slot for: " << this_thread::get_id() << endl;
                old_head = slots_head.load(memory_order::memory_order_acquire);
            }
        });

        auto idx = old_head->idx;
        // delete not working yet :/
        // delete_slot(old_head);
        return idx;
    }

    void release_slot(int slot_idx)
    {
        exec_delete_safe([this, slot_idx]() {
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
        });
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

    /**
     * @brief This Method takes a function and always calls it in delete-safe state. This means, that during this execution it is not possible to delete some data.
     * Unfortunately, I was not able to synchronize the threads in that way, that the delete was used correctly. 
     * I therefore had to disable the functionality and write a destructor.
     * 
     * @tparam Functor for passing a function
     * @param func The function, that should be executed, while the allocator is in a non-deletable state.
     */
    template <typename Functor>
    void exec_delete_safe(Functor func)
    {
        //setDeletable(false);
        func();
        //setDeletable(true);
    }

    bool isDeletable()
    {
        return deletable.load(memory_order::memory_order_acquire);
    }

    void setDeletable(bool del)
    {
        auto old_del = isDeletable();
        while (!deletable.compare_exchange_weak(old_del, del, memory_order::memory_order_release))
        {
            old_del = isDeletable();
        }
    }

    void delete_slot(slot *slot)
    {
        while (!isDeletable())
        {
            this_thread::yield();
            //cout << "Wait for delete" << endl;
        }

        exec_delete_safe([&slot]() {
            if (slot && slot->next)
            {
                free(slot);
            }
        });
    }
};