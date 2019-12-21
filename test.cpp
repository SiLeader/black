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

#include <gtest/gtest.h>

#include <forward_list>
#include <list>

#include "black.hpp"

namespace {
    TEST(object, single) {
        black::BlockAllocator<int> allocator;

        EXPECT_NE(allocator.allocate(1), nullptr);
    }

    TEST(object, multiple) {
        black::BlockAllocator<int> allocator;

        auto p1 = allocator.allocate(1);
        auto p2 = allocator.allocate(1);
        auto p3 = allocator.allocate(1);

        EXPECT_NE(p1, nullptr);
        EXPECT_NE(p2, nullptr);
        EXPECT_NE(p3, nullptr);

        EXPECT_NE(p1, p2);
        EXPECT_NE(p2, p3);
        EXPECT_NE(p3, p1);
    }

    TEST(array, single) {
        black::BlockAllocator<int> allocator;

        EXPECT_NE(allocator.allocate(10), nullptr);
    }

    TEST(array, multiple) {
        black::BlockAllocator<int> allocator;

        auto a1 = allocator.allocate(10);
        auto a2 = allocator.allocate(10);
        auto a3 = allocator.allocate(10);

        EXPECT_NE(a1, nullptr);
        EXPECT_NE(a2, nullptr);
        EXPECT_NE(a3, nullptr);

        EXPECT_NE(a1, a2);
        EXPECT_NE(a2, a3);
        EXPECT_NE(a3, a1);
    }

    TEST(feature, expand) {
        using Allocator = black::BlockAllocator<int>;
        constexpr auto kCount = Allocator::AllocatorSubsystemType::kAllocatableObjectCount;

        Allocator allocator;

        for (std::size_t i = 0; i < kCount * 5; ++i) {
            EXPECT_NE(allocator.allocate(1), nullptr);
        }
    }

    TEST(stl, list) {
        std::list<int, black::BlockAllocator<int>> ls;
        for (std::size_t i = 0; i < 1000; ++i) {
            ls.emplace_back(i);
        }

        for (std::size_t i = 0; i < ls.size(); ++i) {
            EXPECT_EQ(ls.front(), i);
            ls.pop_front();
        }
    }

    TEST(stl, forward_list) {
        std::forward_list<int, black::BlockAllocator<int>> ls;
        for (std::size_t i = 1; i <= 1000; ++i) {
            ls.emplace_front(i);
        }

        for (std::size_t i = 0; i < 1000; ++i) {
            EXPECT_EQ(ls.front(), 1000 - i);
            ls.pop_front();
        }
    }
} // namespace
