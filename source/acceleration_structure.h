#pragma once

#include "maths.h"

#include <vector>

class AccelerationStructurePrimitive {
public:
    const aabbox3& get_bounds() const {
        return m_bounds;
    }

protected:
    aabbox3 m_bounds;
    void* m_node;

    friend class LinearAccelerationStructure;
    friend class OctreeAccelerationStructure;
    friend class QuadtreeAccelerationStructure;
};

class AccelerationStructure {
public:
    virtual void add(AccelerationStructurePrimitive& primitive) = 0;
    virtual void remove(AccelerationStructurePrimitive& primitive) = 0;
    virtual void update(AccelerationStructurePrimitive& primitive) = 0;

    virtual void query(const aabbox3& aabbox, std::vector<AccelerationStructurePrimitive*>& output) const = 0;
    virtual void query(const frustum& frustum, std::vector<AccelerationStructurePrimitive*>& output) const = 0;
};
