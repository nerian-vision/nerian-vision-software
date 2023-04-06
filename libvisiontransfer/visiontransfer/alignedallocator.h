/*******************************************************************************
 * Copyright (c) 2022 Nerian Vision GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/

#ifndef ALIGNEDALLOCATOR_H
#define ALIGNEDALLOCATOR_H

#include <cstdlib>
#include <memory>
#include <limits>

namespace visiontransfer {
namespace internal {

/**
 * \brief STL-compatible allocator for memory-aligned allocations
 *
 * This is a helper class that is used internally for allocating memory
 * that can be used with aligned SSE / AVX instructions.
 */
template<typename T, int alignment = 32>
class AlignedAllocator {
public :
    // Typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind {
        typedef AlignedAllocator<U> other;
    };

    explicit AlignedAllocator() {}
    ~AlignedAllocator() {}
    explicit AlignedAllocator(AlignedAllocator const&) {}
    template<typename U>
    explicit AlignedAllocator(AlignedAllocator<U> const&) {}

    // Address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    // Memory allocation
    pointer allocate(size_type cnt, typename std::allocator<void>::const_pointer = 0) {
        // Allocate memory and align it
        unsigned char* ptr = new unsigned char[sizeof(T) * cnt + (alignment-1) + 1];
        unsigned char* alignedPtr = reinterpret_cast<unsigned char*>((size_t(ptr + 1) + alignment-1) & -alignment);

        // Store offset in allocated memory area
        alignedPtr[-1] = static_cast<unsigned char>(alignedPtr - ptr);

        return reinterpret_cast<pointer>(alignedPtr);
    }

    void deallocate(pointer p, size_type) {
        if(p != nullptr) {
            // Get address of unaligned pointer
            unsigned char* alignedPtr = reinterpret_cast<unsigned char*>(p);
            unsigned char* unalignedPtr = alignedPtr - alignedPtr[-1];

            // Delete it
            ::operator delete[](unalignedPtr);
        }
    }

    // Size
    size_type max_size() const {
        return (std::numeric_limits<size_type>::max)() / sizeof(T);
    }

    // Construction
    void construct(pointer p, const T& t) {
        new(p) T(t);
    }

    // Destruction
    void destroy(pointer p) {
        p->~T();
    }
};

}} // namespace

#endif
