//   Copyright 2019 SiLeader and Cerussite.
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include <chrono>
#include <vector>

#include <cxxabi.h>

#include "black.hpp"

namespace {
    template <template <class> class Allocator>
    std::chrono::milliseconds doTestOnlyAllocate(std::size_t dataLength, std::size_t count,
                                                 std::size_t repeatedCount) {
        auto begin = std::chrono::system_clock::now();
        Allocator<int> allocator;

        for (std::size_t ri = 0; ri < repeatedCount; ++ri) {
            for (std::size_t i = 0; i < count; ++i) {
                allocator.deallocate(allocator.allocate(dataLength), dataLength);
            }
        }
        auto end = std::chrono::system_clock::now();

        return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    }
} // namespace

static constexpr std::size_t kRepeated = 10000000;
static constexpr std::size_t kCount = 10;
static constexpr std::size_t kArrayLength = 5;

template <class T>
using ObjectBlack =
    black::BlockAllocator<T, black::subsystems::LinkedListAllocationSubsystem<T, 64>>;
template <class T>
using ArrayBlack =
    black::BlockAllocator<T, black::subsystems::LinkedListAllocationSubsystem<T, 64>>;

template <class T>
using ObjectBlack2 = black::BlockAllocator<T, black::subsystems::BitAllocationSubsystem<T>>;
template <class T>
using ArrayBlack2 = black::BlockAllocator<T, black::subsystems::BitAllocationSubsystem<T>>;

int main() {
    int stat;
#define DO(alloc, length)                                                                          \
    do {                                                                                           \
        auto name = abi::__cxa_demangle(typeid(alloc<int>).name(), nullptr, nullptr, &stat);       \
        std::cout << name << ": " << doTestOnlyAllocate<alloc>(length, kCount, kRepeated).count()  \
                  << std::endl;                                                                    \
        /*free(name);*/                                                                            \
    } while (0)

    std::cout << "Only allocate object" << std::endl;
    DO(std::allocator, 1);
    DO(ObjectBlack, 1);
    DO(ObjectBlack2, 1);

    std::cout << std::endl;

    std::cout << "Only allocate array" << std::endl;
    DO(std::allocator, kArrayLength);
    DO(ArrayBlack, kArrayLength);
    DO(ArrayBlack2, kArrayLength);
}
