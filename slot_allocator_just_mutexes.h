#include <thread>
#include <vector>
#include <mutex>
#include <cassert>

#if !defined(SLOT_ALLOCATOR_J_MUTEXES)
#define SLOT_ALLOCATOR_J_MUTEXES

#endif // SLOT_ALLOCATOR_J_MUTEXES

using namespace std;

struct slot_allocator_just_mutexes
{
    slot_allocator_just_mutexes() : locks(num_slots) {}

    int acquire_slot()
    {
        for (;;)
        {
            for (int i = 0; i < num_slots; ++i)
            {
                if (locks[i].try_lock())
                {
                    return i;
                }
            }
        }
    }

    void release_slot(int slot)
    {
        locks[slot].unlock();
    }

private:
    int num_slots = 10;
    vector<mutex> locks;
};