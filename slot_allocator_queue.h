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
        {
            slots.push(i);
        }
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
        assertSlots(slot);
        m.unlock();
    }

private:
    int num_slots = 10;
    queue<int> slots;
    mutex m;

    void assertSlots(int insertedVal) const
    {
        // slots are not allowed to be inserted twice. So the queue can never be larger than num_slots
        assert(slots.size() <= num_slots);

        // the newest inserted value should always be on the back of the queue
        auto newestVal = slots.back();
        assert(insertedVal == newestVal);

        /* 
            This code works all of the time.
            However, the use of a queue, in combination with a mutex lock, has a negative impact on performance. 
             - With assertions 40 sec.
             - Without assertions 20 sec.
        */
    }
};
