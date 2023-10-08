#pragma once

#include "Engone/PlatformLayer.h"
#include "Engone/Asserts.h"

struct LinearAllocator {
    LinearAllocator() = default;
    ~LinearAllocator() {
        cleanup();
    }
        
    u8* data = nullptr;;
    int used = 0;
    int max = 0;
    
    void init(int max) {
        Assert(!data);
        data = (u8*)engone::Allocate(max);
        Assert(data);
        used = 0;
        this->max = max;
    }
    void cleanup() {
        engone::Free(data, max);
        data = nullptr;
        max = 0;
        used = 0;
    }
    u8* allocate(int size) {
        // linear allocator can't reserve because it would invalidate the returned pointers.
        Assert(used + size <= max);
        u8* ptr = data + used;
        used += size;
        return ptr;
    }
    void reset() {
        used = 0;   
    }
};