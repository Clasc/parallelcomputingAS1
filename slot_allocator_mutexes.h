#include <thread>
#include <vector>
#include <queue>
#include <iostream>
#include <mutex>
#include <cassert>
#include <chrono>

#if !defined(SLOT_ALLOCATOR_MUTEXES)
#define SLOT_ALLOCATOR_MUTEXES

#endif // SLOT_ALLOCATOR_MUTEXES

using namespace std;

struct slot_allocator_mutexes
{
    slot_allocator_mutexes() : slots(num_slots, false), locks(num_slots) {}

    int acquire_slot()
    {
        for (;;)
        {
            for (int i = 0; i < num_slots; ++i)
            {
                locks[i].lock();
                vector<bool>::reference slot_ref = slots[i];
                if (slot_ref == false)
                {
                    slot_ref = true;
                    locks[i].unlock();
                    return i;
                }
                locks[i].unlock();
            }
        }
    }

    void release_slot(int slot)
    {
        locks[slot].lock();
        assert(slots[slot] == true);
        slots[slot] = false;
        locks[slot].unlock();
    }

private:
    int num_slots = 10;
    vector<bool> slots;
    vector<mutex> locks;

    /**
     * Locking only the slots with mutexes is not enough. 
     * Other parts of the vector can still be changed. 
     * And it has a big performance impact locking so many mutexes.
     * */
};