#pragma once

#include <cassert>
#include <cstdlib>

class CountMemoryResource {
public:
    void* allocate(size_t size) {
        allocated += size;
        return std::malloc(size);
    }

    void deallocate(void* memory, size_t size) {
        assert(allocated >= size);
        allocated -= size;
        return std::free(memory);
    }

    size_t allocated = 0;
};

template <typename T>
class CountAllocator {
public:
    using value_type = T;

    CountAllocator(CountMemoryResource& memory_resource_)
        : memory_resource(memory_resource_)
    {
    }

    template <typename U>
    CountAllocator(const CountAllocator<U>& allocator)
        : memory_resource(allocator.memory_resource)
    {
    }

    T* allocate(size_t count) {
        return static_cast<T*>(memory_resource.allocate(sizeof(T) * count));
    }

    void deallocate(T* memory, size_t count) {
        memory_resource.deallocate(memory, sizeof(T) * count);
    }

    bool operator==(const CountAllocator& other) const {
        return &memory_resource == &other.memory_resource;
    }

    bool operator!=(const CountAllocator& other) const {
        return &memory_resource != &other.memory_resource;
    }

    CountMemoryResource& memory_resource;
};
