#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <cassert>

#if !defined(SLOT_ALLOCATOR_QUEUE)
#define SLOT_ALLOCATOR_QUEUE

#endif // SLOT_ALLOCATOR_QUEUE

using namespace std;

struct slot_allocator_queue
{
    slot_allocator_queue()
    {
        for (int i = 0; i < 10; ++i)
            slots.push(i);
    }

    int acquire_slot()
    {
        for (;;)
        {
            m.lock();
            if (!slots.empty())
            {
                int slot = slots.front();
                slots.pop();
                m.unlock();
                return slot;
            }
            m.unlock();
        }
    }

    void release_slot(int slot)
    {
        m.lock();
        slots.push(slot);
        m.unlock();
    }

private:
    int num_slots = 10;
    queue<int> slots;
    mutex m;
};
