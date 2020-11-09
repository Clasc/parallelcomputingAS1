#include <thread>
#include <vector>
#include <iostream>
#include <mutex>
#include <cassert>
#include <queue>
#include <chrono>
#include <atomic>
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
				However, it is not guaranteeing mutual exclusion. 
                The code has a similar behaviour when using no timeout.
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
             - With assertions and cout statements 40 sec.
             - Without assertions with cout statements 20 sec.
             - Without assertions, no cout statements 0.962354 sec. -- compared to 0.043077 with just mutexes
        */
    }
};

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
     * Other parts of the vector can still be changed -> There is a possibility, that a Thread is acquiring a slot, while another is releasing it at the same time. 
     * Assert in release_slot asserts to false, since the value slot has alreay been changed.
     * It has a big performance impact locking so many mutexes. It's more efficient only having one mutex for the whole vector.
     * */
};

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
        // code fix: add mutex unlock
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
     * Therefore, we need a lock also when releasing a slot.
     **/
};

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
        // if the lock can be locked again, it was already unlocked and therefore should assert to false
        // assert can be uncommented to test performance
        assert(locks[slot].try_lock() == false);
        locks[slot].unlock();
    }

private:
    int num_slots = 10;
    vector<mutex> locks;

    /**
     * This allocator worked from the get-go.
     * The added assert checks, if the slot to release is already unlocked. If so, it asserts to false and the execution is stopped.
     * This allocator however never asserted false. This means it is working correctly.
     * It is also the one with the best performance. Making it a good code to compare with our atomic solution.
     */
};

struct slot_allocator_atomic_array
{
    slot_allocator_atomic_array()
    {
        slots = new atomic<bool>[num_slots];
    }

    ~slot_allocator_atomic_array()
    {
        delete[] slots;
    }

    int acquire_slot()
    {
        for (;;)
        {
            for (auto i{0}; i < num_slots; i++)
            {
                auto is_locked = slots[i].load(memory_order::memory_order_acquire);
                if (is_locked)
                {
                    continue;
                }

                while (!slots[i].compare_exchange_weak(is_locked, true, memory_order::memory_order_release))
                {
                    is_locked = slots[i].load(memory_order::memory_order_acquire);
                }
                return i;
            }
            this_thread::yield();
        }
    }

    void release_slot(int slot_idx)
    {
        bool is_locked = slots[slot_idx].load(memory_order::memory_order_acquire);
        while (!slots[slot_idx].compare_exchange_weak(is_locked, false, memory_order::memory_order_release))
        {
            is_locked = slots[slot_idx].load(memory_order::memory_order_acquire);
            this_thread::yield();
        }
    }

private:
    int num_slots = 10;
    atomic<bool> *slots;
    /**
     * Performance:
     * Depending on the system this is compiled and run on.
     * On my machine (Macbook pro 2016) -
     * - Just Mutexes take around ~0.035 sec.
     * - Atomic Array take around ~0.07 sec.
     * 
     * On ALMA we are getting a different result.
     * - Just Mutexes take around 0.3 sec.
     * - Atomic Array takes around 0.1 sec.
     */
};

//typedef slot_allocator_sleep slot_allocator;
//typedef slot_allocator_mutex slot_allocator;
//typedef slot_allocator_queue slot_allocator;
//typedef slot_allocator_mutexes slot_allocator;
//typedef slot_allocator_just_mutexes slot_allocator;
//typedef slot_allocator_atomic slot_allocator;
//typedef slot_allocator_atomic_vector slot_allocator;
typedef slot_allocator_atomic_array slot_allocator;

int main()
{
    int num_threads = 8;
    int repeats = 100000;
    slot_allocator alloc;

    mutex cout_lock; //this could be used to print out the slot numbers in an orderly manner
    vector<thread> threads;
    auto t1 = chrono::high_resolution_clock::now();

    for (int t = 0; t < num_threads; ++t)
    {
        threads.push_back(thread([&]() {
            for (int r = 0; r < repeats; ++r)
            {
                int slot = alloc.acquire_slot();

                // cout_lock.lock();
                // cout << "slot :" << slot << endl;
                // cout << endl;
                // cout_lock.unlock();

                alloc.release_slot(slot);
            }
        }));
    }

    for (auto &t : threads)
    {
        t.join();
    }
    auto t2 = chrono::high_resolution_clock::now();
    cout << chrono::duration<double>(t2 - t1).count();
}
