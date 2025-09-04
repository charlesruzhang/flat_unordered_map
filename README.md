In general, the std::unordered_map is known for having linked-list structures in the buckets. Even if we fix the total bucket size, there will be pointer chasing when looking for an entry for a certain bucket. Pointer chasing will hugely increase the cache miss rates so the tail latency can be unpredictable. So the updated version of flat_unordered_map uses a flat array for all the entries and used some extra memory for the prefix sum computation so that it's easier for us to know the location to start scanning for each hashed value. 

An alternative approach would be just to keep a vector<vector<pair<K, V>> in our memory, which saves us the extra prefix-sum memory but accessing a random subvector in a vector of vectors will also cause cache misses and potentially increase the tail latency. This alternative approach would still be a better approach than the unordered_map one because it has a flat structure for each of the buckets, enhancing the cache performances.

I used ChatGPT in multiple scenarios: I searched for how to include Eq and Hash functions in a template, asked it for tiny bugs fixes and tiny optimizations like : adding "const" quantifier to the variable, and it suggested me doing the ticks function in line 9 because the compiler may switch the ordering of __rdtsc();

I also copied my cout function from my previous orderbook project to print p50/p75/p99 time in ns of the operations.

I added one big test case and checked it against the correct result for both keys in the map and keys not in the map, timed it in a different test against unordered_map and one can run the code by doing g++ -std=c++2a flat.cpp -o flat; ./flat. I also removed the -O3 flag because it will automatically optimize my tests, which I don't want to occur.

Since I am having so few free hours today, and I don't think I have too many free hours, please let me know if I can further improve this code and I will work on it this weekend if necessary.