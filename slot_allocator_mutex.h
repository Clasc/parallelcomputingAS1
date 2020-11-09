#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <cassert>

using namespace std;

struct slot_allocator_mutex
{
    slot_allocator_mutex() : slots(num_slots, false) {}
    int acquire_slot()
    {
        for (;;)
        {
            m.lock();
            for (int i = 0; i < num_slots; ++i)
            {
                if (slots[i] == false)
                {
                    slots[i] = true;
                    m.unlock();
                    return i;
                }
            }
            m.unlock();
        }
    }

    void release_slot(int slot)
    {
        // code fix: add mutex lock
        m.lock();
        assertSlots(slot);
        slots[slot] = false;
        m.unlock();
    }

private:
    int num_slots = 10;
    vector<bool> slots;
    mutex m;

    void assertSlots(int slotToRelease)
    {
        assert(slots[slotToRelease] == true);
    }

    /**
     * It runs, but after a few times, we do get the same slot all the time
     * Why? --> Release also needs a mutex lock!
     * If there is no mutex lock around the release, multiple threads can write simultaneously in the slot.
     * There should not be two threads releasing the same slot, because we make sure during acquiring . 
     * However, it is possible, that a slot is released, and acquired at the same time.
     * Therefore, we need a lock also when releasing a slot
     **/
};
