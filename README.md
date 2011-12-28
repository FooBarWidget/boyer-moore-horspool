Introduction
============
This repository contains various C++ implementations of the Boyer-Moore string search algorithm and derivative algorithms. These family of algorithms allow fast searching of substrings, much faster than `strstr()` and `memmem()`. The longer the substring, the faster the algorithms work. The implementations are written to be both efficient and minimalist so that you can easily incorporate them in your own code.

This blog post has more information: http://blog.phusion.nl/2010/12/06/efficient-substring-searching/


Files
-----

### Horspool.cpp
Implements Boyer-Moore-Horspool.
HorspoolTest.cpp is the unit test file.

### BoyerMooreAndTurbo.cpp
Implements Boyer-Moore and Turbo Boyer-Moore. No special test files, but they're used in the benchmark program which serves as a basic sanity test.

### StreamBoyerMooreHorspool.h
A special Boyer-Moore-Horspool implementation that supports "streaming" input. Instead of supplying the entire haystack at once, you can supply the haystack piece-by-piece. This makes it especially suitable for parsing data that you may receive over the network. This implementation also contains various memory and CPU optimizations, allowing it to be slightly faster and to use less memory than Horspool.cpp. See the file for detailed documentation.

Unit tests are in StreamTest.cpp.

### benchmark.cpp
Benchmark program. Used in combination with the `run_benchmark` Rake task.

### TestMain.cpp
Unit test runner program.


Testing and benchmarking
------------------------

You need Ruby and Rake. To compile the unit tests:

    rake test
    ./test

To run the benchmarks:

    rake run_benchmark


Which algorithm to use?
-----------------------
I've found that, on average, Boyer-Moore-Horspool performs best thanks to its
simple inner loop which can be heavily optimized. It has pretty bad worst-case
performance but the worst case (or even bad cases) almost never occur in practice.
