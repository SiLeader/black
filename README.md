# Black - Block allocator

&copy; 2019 SiLeader and Cerussite.

## feature
+ C++ allocator
+ only one header file (black.hpp)
+ 2 allocation system
  + bit allocation subsystem (black::subsystems::BitAllocationSubsystem) (default)
  + linked list allocation subsystem (black::subsystems::LinkedListAllocationSubsystem)
+ faster than std::allocator
+ optimized for the fixed type
+ std::list, std::forward_list support

## how to use
```c++
#include <black.hpp>

// std::list
std::list<int, black::BlockAllocator<int>> ls;

// std::forward_list
std::forward_list<
        int,
        black::BlockAllocator<int, black::subsystems::LinkedListAllocationSubsystem<int, 64>>> ls;
```

## speed
source code: `speed_test.cpp`

### object allocation
| ----- | std::allocator | black (Bit) | black (LinkedList) |
|:-----:|:--------------:|:-----------:|:------------------:|
|1|1400|467|551|
|2|1274|467|599|
|3|1263|472|536|
|4|1309|478|530|
|5|1342|485|604|
| average | 1317,6 | 473.8 | 564 |
| median | 1309 | 471 | 551 |

+ black(Bit) is about 2.78 times faster than std::allocator.

### array allocation
| ----- | std::allocator | black (Bit) | black (LinkedList) |
|:-----:|:--------------:|:-----------:|:------------------:|
|1|1288|471|1078|
|2|1302|475|1085|
|3|1300|492|1478|
|4|1343|482|1332|
|5|1356|479|1249|
| average |1317.8|479.8|1244.4|
| median |1302|479|1249|

+ black(Bit) is about 2.75 times as fast as std::allocator.

### env
+ CPU: AMD Ryzen 7 1700
+ memory: 15.7 GB
+ OS: Linux Mint 19.2
+ compiler: clang
+ standard c++ libraries: libstdc++

## license
Apache License 2.0
