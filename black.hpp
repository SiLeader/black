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

#pragma once

#include <bitset>
#include <cstddef>
#include <iostream>
#include <type_traits>

#ifndef likely
#define _likely_black_defined 1
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define _unlikely_black_defined 1
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

namespace black {
    namespace subsystems {
        namespace detail {
            template <std::size_t ObjectSize, std::size_t CurrentSize, std::size_t Align>
            struct BlockSizeImpl {
                static constexpr std::size_t value =
                    BlockSizeImpl<ObjectSize, CurrentSize + Align, Align>::value;
            };
            template <std::size_t Size, std::size_t Align> struct BlockSizeImpl<Size, Size, Align> {
                static constexpr std::size_t value = Size;
            };

            template <std::size_t Size, std::size_t Align> struct BlockSize {
                static constexpr std::size_t value = BlockSizeImpl<Size, Align, Align>::value;
            };
        } // namespace detail

        template <class T, std::size_t ObjectCount> class LinkedListAllocationSubsystem {
        public:
            static constexpr std::size_t kBlockSize =
                detail::BlockSize<sizeof(T), alignof(T)>::value;
            static constexpr std::size_t kAllocatableObjectCount = ObjectCount;
            static constexpr std::size_t kBucketSize = kAllocatableObjectCount * kBlockSize;

            using value_type = T;

            template <class U> struct rebind {
                using other = LinkedListAllocationSubsystem<U, ObjectCount>;
            };

        private:
            struct Bucket {
                char block[kBlockSize];
            };

            struct Node {
                Node *next;
            };

        private:
            /// free list (forward linked list)
            Node _freeList[kAllocatableObjectCount];
            /// first free node
            Node *_freeListTop;

            /// bucket
            typename std::aligned_storage<kBucketSize, alignof(T)>::type _bucket;
            /// pointer to bucket top
            Bucket *_first;

        public:
            LinkedListAllocationSubsystem() noexcept
                : _freeListTop(_freeList)
                , _bucket()
                , _first(reinterpret_cast<Bucket *>(&_bucket)) {
                for (std::size_t i = 1; i < kAllocatableObjectCount; ++i) {
                    _freeList[i - 1].next = _freeList + i;
                }
            }

            LinkedListAllocationSubsystem(const LinkedListAllocationSubsystem &) = delete;
            LinkedListAllocationSubsystem(LinkedListAllocationSubsystem &&) = delete;

            LinkedListAllocationSubsystem &
            operator=(const LinkedListAllocationSubsystem &) = delete;
            LinkedListAllocationSubsystem &operator=(LinkedListAllocationSubsystem &&) = delete;

            ~LinkedListAllocationSubsystem() = default;

        public:
            /// allocate memory
            /// \param n object count
            /// \return pointer to allocated area. nullptr if no more allocatable area
            T *allocate(std::size_t n) noexcept {
                if (unlikely(_freeListTop == nullptr)) {
                    // cannot allocate
                    return nullptr;
                }

                auto prev = _freeListTop;      // previous free node
                auto candidate = _freeListTop; // head of continuous free area
                auto tail = _freeListTop;      // tail of continuous free area
                std::size_t areaLength = 1;

                while (areaLength < n) {
                    if (unlikely(tail->next == nullptr))
                        return nullptr;

                    if ((tail + 1) != tail->next) {
                        // non continuous area
                        prev = tail;
                        candidate = tail->next;
                        tail = candidate;
                        areaLength = 1;
                    } else {
                        tail = tail->next;
                        areaLength++;
                    }
                }

                if (likely(candidate == _freeListTop)) {
                    _freeListTop = tail->next;
                } else {
                    prev->next = tail->next;
                }

                auto index = candidate - _freeList;
                return reinterpret_cast<T *>(&_first[index]);
            }

            bool inRange(const T *ptr) {
                if (unlikely(ptr == nullptr))
                    return false;
                auto index =
                    static_cast<std::size_t>(reinterpret_cast<const Bucket *>(ptr) - _first);
                return 0 <= index && index < kAllocatableObjectCount;
            }

            /// deallocate area
            /// \param ptr area to deallocate
            /// \param n object count
            bool deallocate(const T *ptr, std::size_t n) noexcept {
                if (!inRange(ptr)) {
                    return false;
                }
                auto index = reinterpret_cast<const Bucket *>(ptr) - _first;

                auto node = _freeList + index;
                if (node < _freeListTop) {
                    auto currentTop = _freeListTop;
                    _freeListTop = node;
                    for (std::size_t i = 0; i < n; ++i) {
                        node = node->next;
                    }
                    node->next = currentTop;

                } else {
                    auto nextNode = _freeListTop->next;
                    auto prevNode = _freeListTop;
                    while (nextNode != nullptr && node < nextNode) {
                        prevNode = nextNode;
                        nextNode = nextNode->next;
                    }

                    prevNode->next = node;

                    for (std::size_t i = 0; i < n; ++i) {
                        node = node->next;
                    }
                    node->next = nextNode;
                }
                return true;
            }
        };

        namespace detail {
            template <std::size_t N> struct NBitmask {
                static constexpr std::uint_fast64_t value =
                    (1ull << (N - 1) | NBitmask<N - 1>::value);
            };
            template <> struct NBitmask<0> { static constexpr std::uint_fast64_t value = 1; };
        } // namespace detail

        /// Block allocator subsystem
        /// This subsystem use bit operations to manage free areas.
        /// \tparam T object type
        template <class T> class BitAllocationSubsystem {
        public:
            static constexpr std::size_t kBlockSize =
                detail::BlockSize<sizeof(T), alignof(T)>::value;
            static constexpr std::size_t kAllocatableObjectCount = 64;
            static constexpr std::size_t kBucketSize = kAllocatableObjectCount * kBlockSize;

            using value_type = T;

            template <class U> struct rebind { using other = BitAllocationSubsystem<U>; };

        private:
            static std::uint_fast64_t NBit(std::size_t n) {
                switch (n) {
#define C(n)                                                                                       \
    case n:                                                                                        \
        return detail::NBitmask<n>::value
                    C(1);
                    C(2);
                    C(3);
                    C(4);
                    C(5);
                    C(6);
                    C(7);
                    C(8);
                    C(9);
                    C(10);
                    C(11);
                    C(12);
                    C(13);
                    C(14);
                    C(15);
                    C(16);
                    C(17);
                    C(18);
                    C(19);
                    C(20);
                    C(21);
                    C(22);
                    C(23);
                    C(24);
                    C(25);
                    C(26);
                    C(27);
                    C(28);
                    C(29);
                    C(30);
                    C(31);
                    C(32);
                    C(33);
                    C(34);
                    C(35);
                    C(36);
                    C(37);
                    C(38);
                    C(39);
                    C(40);
                    C(41);
                    C(42);
                    C(43);
                    C(44);
                    C(45);
                    C(46);
                    C(47);
                    C(48);
                    C(49);
                    C(50);
                    C(51);
                    C(52);
                    C(53);
                    C(54);
                    C(55);
                    C(56);
                    C(57);
                    C(58);
                    C(59);
                    C(60);
                    C(61);
                    C(62);
                    C(63);
                    C(64);
                default:
                    return 0;
#undef C
                }
            }

        private:
            struct Bucket {
                char block[kBlockSize];
            };

        private:
            std::uint_fast64_t _freeBlockList;

            /// bucket
            typename std::aligned_storage<kBucketSize, alignof(T)>::type _bucket;
            /// pointer to bucket top
            Bucket *_first;

        public:
            BitAllocationSubsystem() noexcept
                : _freeBlockList()
                , _bucket()
                , _first(reinterpret_cast<Bucket *>(&_bucket)) {}

            BitAllocationSubsystem(const BitAllocationSubsystem &) = delete;
            BitAllocationSubsystem(BitAllocationSubsystem &&) = delete;

            BitAllocationSubsystem &operator=(const BitAllocationSubsystem &) = delete;
            BitAllocationSubsystem &operator=(BitAllocationSubsystem &&) = delete;

            ~BitAllocationSubsystem() = default;

        public:
            /// allocate memory
            /// \param n object count
            /// \return pointer to allocated area. nullptr if no more allocatable area
            T *allocate(std::size_t n) noexcept {
                if (unlikely(_freeBlockList == 0xffffffffffffffff))
                    return nullptr;

                auto bitmask = NBit(n);
                for (std::size_t i = 0; i < (kAllocatableObjectCount - n); ++i) {
                    if ((_freeBlockList & bitmask) == 0) {
                        _freeBlockList |= bitmask;
                        return reinterpret_cast<T *>(_first + i);
                    }
                    bitmask <<= 1u;
                }
                return nullptr;
            }

            /// deallocate area
            /// \param ptr area to deallocate
            /// \param n object count
            bool deallocate(const T *ptr, std::size_t n) noexcept {
                const auto index = reinterpret_cast<const Bucket *>(ptr) - _first;
                if (unlikely(index < 0))
                    return false;
                if (unlikely(index >= 64))
                    return false;

                _freeBlockList &= ~(NBit(n) << index);
                return true;
            }
        };
    } // namespace subsystems

    template <class T, class Subsystem = subsystems::BitAllocationSubsystem<T>>
    class BlockAllocator {
    public:
        using AllocatorSubsystemType = Subsystem;
        static constexpr std::size_t kBlockSize = AllocatorSubsystemType::kBlockSize;

        using value_type = T;
        template <class U> struct rebind {
            using other = BlockAllocator<U, typename Subsystem::template rebind<U>::other>;
        };

    private:
        struct Node {
            Node *next;
            AllocatorSubsystemType allocator;
        };

        using AllocatorType = std::allocator<Node>;

    private:
        AllocatorType _internalAllocator;
        Node *_allocators;

    private:
        Node *allocateNewNode(Node *tail) {
            auto newNode = _internalAllocator.allocate(1);
            new (newNode) Node();

            tail->next = newNode;
            newNode->next = nullptr;

            return newNode;
        }

    public:
        BlockAllocator()
            : _allocators(_internalAllocator.allocate(1)) {
            new (_allocators) Node();
            _allocators->next = nullptr;
        }

        BlockAllocator(const BlockAllocator &) = delete;
        BlockAllocator(BlockAllocator &&) = delete;

        BlockAllocator &operator=(const BlockAllocator &) = delete;
        BlockAllocator &operator=(BlockAllocator &&) = delete;

        ~BlockAllocator() {
            auto node = _allocators;
            while (node != nullptr) {
                auto next = node->next;
                node->~Node();
                _internalAllocator.deallocate(node, 1);
                node = next;
            }
        }

    public:
        T *allocate(std::size_t n) {
            auto allocator = _allocators;
            while (true) {
                auto ptr = allocator->allocator.allocate(n);
                if (ptr)
                    return ptr;

                if (allocator->next == nullptr)
                    break;
                allocator = allocator->next;
            }

            auto node = allocateNewNode(allocator);

            return node->allocator.allocate(1);
        }

        void deallocate(T *ptr, std::size_t n) {
            auto allocator = _allocators;
            while (allocator != nullptr) {
                if (allocator->allocator.deallocate(ptr, n)) {
                    return;
                }
                allocator = allocator->next;
            }
        }
    };
} // namespace black

#ifdef _likely_black_defined
#undef _likely_black_defined
#undef likely
#endif

#ifdef _unlikely_black_defined
#undef _unlikely_black_defined
#undef unlikely
#endif
