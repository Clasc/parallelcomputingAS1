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
        slots[slot] = false;
    }

private:
    int num_slots = 10;
    vector<bool> slots;
    mutex m;
};
