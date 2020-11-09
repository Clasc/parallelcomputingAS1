#include <thread>
#include <vector>
#include <cassert>
#include <chrono>

#if !defined(SLOT_ALLOCATOR_SLEEP)
#define SLOT_ALLOCATOR_SLEEP

#endif // SLOT_ALLOCATOR_SLEEP

using namespace std;
using namespace std::chrono_literals;

struct slot_allocator_sleep
{
    slot_allocator_sleep() : slots(num_slots, false) {}

    int acquire_slot()
    {
        for (;;)
        {
            for (int i = 0; i < num_slots; ++i)
            {
                if (slots[i] == false)
                {
                    slots[i] = true;
                    return i;
                }
            }

            /*
				The sleep function may work for a short perdiod of time. 
				However, it is not guaranteeing mutual exclusion. The code has a similar behaviour when using no timeout.
				All threads are able to access/change the slot array at the same time.
				There is still the possibility that after the sleep, some threads try to acquire a slot at the same time.
                To fix it use a mutex lock or a lock guard.
			*/
            this_thread::sleep_for(100ms);
        }
    }

    void release_slot(int slot)
    {
        assert(slots[slot] == true);
        slots[slot] = false;
    }

private:
    int num_slots = 10;
    vector<bool> slots;
};
