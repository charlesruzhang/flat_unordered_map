Instead of chained buckets and pointer chasing like many std::unordered_map implementations, this map packs entries into one flat array, and uses a small prefix-sum index to locate the contiguous range (bucket) for any hash.

Typical unordered_map buckets are linked lists or node-based structures; probing a key can bounce around memory and blow the cache. While in my implementation we count elements per bucket, compute prefix sums, and scatter entries into one std::vector<std::pair<K,V>>. Each bucket becomes a tight slice you can linearly scan—fast and predictable.

I used ChatGPT in multiple scenarios: I searched for how to include Eq and Hash functions in a template, asked it for tiny bugs fixes and tiny optimizations like : adding "const" quantifier to the variable, and it suggested me doing the ticks function in line 9 because the compiler may switch the ordering of __rdtsc();

I also copied my cout function from my previous orderbook project to print p50/p75/p99 time in cycles of the operations.

I added one big test case and checked it against the correct result for both keys in the map and keys not in the map, timed it in a different test against unordered_map and one can run the code by doing g++ -std=c++2a test.cpp -o test; ./test [Your test size] [Your map size] [Your invalid guard test count]. I also removed the -O3 flag just for testing purposes.
    