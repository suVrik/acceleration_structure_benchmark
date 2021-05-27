#pragma once

#include "acceleration_structure.h"
#include "count_allocator.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

constexpr uint32_t OCTREE_POSITIVE_X = 0;
constexpr uint32_t OCTREE_NEGATIVE_X = 1 << 0;
constexpr uint32_t OCTREE_POSITIVE_Y = 0;
constexpr uint32_t OCTREE_NEGATIVE_Y = 1 << 1;
constexpr uint32_t OCTREE_POSITIVE_Z = 0;
constexpr uint32_t OCTREE_NEGATIVE_Z = 1 << 2;

static float3 OCTREE_EXTENT_FACTORS[] = {
    float3{  1.f,  1.f,  1.f },
    float3{ -1.f,  1.f,  1.f },
    float3{  1.f, -1.f,  1.f },
    float3{ -1.f, -1.f,  1.f },
    float3{  1.f,  1.f, -1.f },
    float3{ -1.f,  1.f, -1.f },
    float3{  1.f, -1.f, -1.f },
    float3{ -1.f, -1.f, -1.f },
};

struct OctreeNode {
    OctreeNode(CountMemoryResource& memory_resource)
        : primitives(memory_resource)
    {
    }

    std::unique_ptr<OctreeNode> children[8];
    std::vector<AccelerationStructurePrimitive*, CountAllocator<AccelerationStructurePrimitive*>> primitives;
    aabbox3 bounds;
};

class OctreeAccelerationStructure : public AccelerationStructure, private OctreeNode {
public:
    OctreeAccelerationStructure(CountMemoryResource& memory_resource_, const float3& center, const float3& extent, uint32_t max_depth)
        : OctreeNode(memory_resource_)
        , memory_resource(memory_resource_)
        , m_max_depth(max_depth)
    {
        assert(extent.x > 0.f);
        assert(extent.y > 0.f);
        assert(extent.z > 0.f);

        bounds.center = center;
        bounds.extent = extent;
    }

    void add(AccelerationStructurePrimitive& primitive) override {
        OctreeNode& node = find_node(primitive.get_bounds(), *this);
        assert(std::find(node.primitives.begin(), node.primitives.end(), &primitive) == node.primitives.end());

        node.primitives.push_back(&primitive);

        primitive.m_node = &node;
    }

    void remove(AccelerationStructurePrimitive& primitive) override {
        OctreeNode* node = static_cast<OctreeNode*>(primitive.m_node);
        assert(node != nullptr);

        auto it = std::find(node->primitives.begin(), node->primitives.end(), &primitive);
        assert(it != node->primitives.end());

        *it = node->primitives.back();
        node->primitives.pop_back();

        primitive.m_node = nullptr;
    }

    void update(AccelerationStructurePrimitive& primitive) override {
        OctreeNode* node = static_cast<OctreeNode*>(primitive.m_node);
        assert(node != nullptr);

        const aabbox3& bounds = primitive.get_bounds();

        if (bounds.center.x - bounds.extent.x <  node->bounds.center.x - node->bounds.extent.x ||
            bounds.center.y - bounds.extent.y <  node->bounds.center.y - node->bounds.extent.y ||
            bounds.center.z - bounds.extent.z <  node->bounds.center.z - node->bounds.extent.z ||
            bounds.center.x + bounds.extent.x >= node->bounds.center.x + node->bounds.extent.x ||
            bounds.center.y + bounds.extent.y >= node->bounds.center.y + node->bounds.extent.y ||
            bounds.center.z + bounds.extent.z >= node->bounds.center.z + node->bounds.extent.z)
        {
            auto it = std::find(node->primitives.begin(), node->primitives.end(), &primitive);
            assert(it != node->primitives.end());

            *it = node->primitives.back();
            node->primitives.pop_back();

            OctreeNode& node = find_node(bounds, *this);
            node.primitives.push_back(&primitive);

            primitive.m_node = &node;
        }
    }

    void query(const aabbox3& aabbox, std::vector<AccelerationStructurePrimitive*>& output) const override {
        collect_primitives(*this, aabbox, output);
    }

    void query(const frustum& frustum, std::vector<AccelerationStructurePrimitive*>& output) const override {
        collect_primitives(*this, frustum, output);
    }

private:
    OctreeNode& find_node(const aabbox3& bounds, OctreeNode& node, uint32_t depth = 0) {
        if (depth >= m_max_depth) {
            return node;
        }

        uint32_t index = 0;

        if (bounds.center.x - bounds.extent.x >= node.bounds.center.x) {
            index |= OCTREE_POSITIVE_X;
        } else if (bounds.center.x + bounds.extent.x < node.bounds.center.x) {
            index |= OCTREE_NEGATIVE_X;
        } else {
            return node;
        }

        if (bounds.center.y - bounds.extent.y >= node.bounds.center.y) {
            index |= OCTREE_POSITIVE_Y;
        } else if (bounds.center.y + bounds.extent.y < node.bounds.center.y) {
            index |= OCTREE_NEGATIVE_Y;
        } else {
            return node;
        }

        if (bounds.center.z - bounds.extent.z >= node.bounds.center.z) {
            index |= OCTREE_POSITIVE_Z;
        } else if (bounds.center.z + bounds.extent.z < node.bounds.center.z) {
            index |= OCTREE_NEGATIVE_Z;
        } else {
            return node;
        }

        std::unique_ptr<OctreeNode>& child = node.children[index];
        if (!child) {
            float extent_x = node.bounds.extent.x / 2.f;
            float extent_y = node.bounds.extent.y / 2.f;
            float extent_z = node.bounds.extent.z / 2.f;

            float center_x = node.bounds.center.x + OCTREE_EXTENT_FACTORS[index].x * extent_x;
            float center_y = node.bounds.center.y + OCTREE_EXTENT_FACTORS[index].y * extent_y;
            float center_z = node.bounds.center.z + OCTREE_EXTENT_FACTORS[index].z * extent_z;

            child = std::unique_ptr<OctreeNode>(new (memory_resource.allocate(sizeof(OctreeNode))) OctreeNode(memory_resource));
            child->bounds = aabbox3{
                float3{ center_x, center_y, center_z },
                float3{ extent_x, extent_y, extent_z }
            };
        }

        return find_node(bounds, *child, depth + 1);
    }

    template <typename Bounds>
    void collect_primitives(const OctreeNode& node, const Bounds& bounds, std::vector<AccelerationStructurePrimitive*>& output) const {
        output.reserve(output.size() + node.primitives.size());

        for (AccelerationStructurePrimitive* primitive : node.primitives) {
            if (intersect(primitive->get_bounds(), bounds)) {
                output.push_back(primitive);
            }
        }

        for (const std::unique_ptr<OctreeNode>& child : node.children) {
            if (child && intersect(child->bounds, bounds)) {
                collect_primitives(*child, bounds, output);
            }
        }
    }

    CountMemoryResource& memory_resource;
    uint32_t m_max_depth;
};
