#pragma once

#include "acceleration_structure.h"
#include "count_allocator.h"

#include <algorithm>
#include <cassert>

class LinearAccelerationStructure : public AccelerationStructure {
public:
    LinearAccelerationStructure(CountMemoryResource& memory_resource)
        : m_primitives(memory_resource)
    {
    }

    void add(AccelerationStructurePrimitive& primitive) override {
        assert(std::find(m_primitives.begin(), m_primitives.end(), &primitive) == m_primitives.end());
        m_primitives.push_back(&primitive);
    }

    void remove(AccelerationStructurePrimitive& primitive) override {
        auto it = std::find(m_primitives.begin(), m_primitives.end(), &primitive);
        assert(it != m_primitives.end());

        *it = m_primitives.back();
        m_primitives.pop_back();
    }

    void update(AccelerationStructurePrimitive& primitive) override {
        // No-op.
    }

    void query(const aabbox3& aabbox, std::vector<AccelerationStructurePrimitive*>& output) const override {
        for (AccelerationStructurePrimitive* primitive : m_primitives) {
            if (intersect(primitive->get_bounds(), aabbox)) {
                output.push_back(primitive);
            }
        }
    }

    void query(const frustum& frustum, std::vector<AccelerationStructurePrimitive*>& output) const override {
        for (AccelerationStructurePrimitive* primitive : m_primitives) {
            if (intersect(primitive->get_bounds(), frustum)) {
                output.push_back(primitive);
            }
        }
    }

private:
    std::vector<AccelerationStructurePrimitive*, CountAllocator<AccelerationStructurePrimitive*>> m_primitives;
};
