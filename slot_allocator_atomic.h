#include <thread>
#include <cassert>
#include <atomic>

#if !defined(SLOT_ALLOCATOR_ATOMIC)
#define SLOT_ALLOCATOR_ATOMIC

#endif // SLOT_ALLOCATOR_ATOMIC

using namespace std;

struct slot_allocator_atomic
{
    slot_allocator_atomic()
    {
        lockedSlots.store(0);
    }

    int acquire_slot()
    {
        while (true)
        {
            if (lockedSlots.load() < num_slots)
            {
                lockedSlots++;
                return lockedSlots.load();
            }
        }
    }

    void release_slot(int slot)
    {
        lockedSlots--;
    }

private:
    int num_slots = 10;
    atomic<int> lockedSlots;
};