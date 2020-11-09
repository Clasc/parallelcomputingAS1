// #include <thread>
// #include <vector>
// #include <iostream>
// #include <mutex>
// #include <cassert>
// #include "slot_allocator_just_mutexes.h"
// #include "slot_allocator_sleep.h"
// #include "slot_allocator_mutexes.h"
// #include "slot_allocator_queue.h"
// #include "slot_allocator_mutex.h"
// #include "slot_allocator_atomic.h"
// #include "slot_allocator_atomic_vector.h"
// #include "slot_allocator_atomic_array.h"

// using namespace std;

// //typedef slot_allocator_sleep slot_allocator;
// //typedef slot_allocator_mutex slot_allocator;
// //typedef slot_allocator_queue slot_allocator;
// //typedef slot_allocator_mutexes slot_allocator;
// //typedef slot_allocator_just_mutexes slot_allocator;
// //typedef slot_allocator_atomic slot_allocator;
// //typedef slot_allocator_atomic_vector slot_allocator;
// typedef slot_allocator_atomic_array slot_allocator;

// int main()
// {
// 	int num_threads = 8;
// 	int repeats = 100000;
// 	slot_allocator alloc;

// 	mutex cout_lock; //this could be used to print out the slot numbers in an orderly manner
// 	vector<thread> threads;
// 	auto t1 = chrono::high_resolution_clock::now();

// 	for (int t = 0; t < num_threads; ++t)
// 	{
// 		threads.push_back(thread([&]() {
// 			for (int r = 0; r < repeats; ++r)
// 			{
// 				int slot = alloc.acquire_slot();

// 				/*
// 				cout_lock.lock();
// 				cout << "slot :" << slot << endl;
// 				cout << endl;
// 				cout_lock.unlock();
// 				*/

// 				alloc.release_slot(slot);
// 			}
// 		}));
// 	}

// 	for (auto &t : threads)
// 	{
// 		t.join();
// 	}
// 	auto t2 = chrono::high_resolution_clock::now();
// 	cout << chrono::duration<double>(t2 - t1).count();
// }
