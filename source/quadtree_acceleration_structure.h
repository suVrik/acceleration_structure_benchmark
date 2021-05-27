#pragma once

#include "acceleration_structure.h"
#include "count_allocator.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

constexpr uint32_t QUADTREE_POSITIVE_X = 0;
constexpr uint32_t QUADTREE_NEGATIVE_X = 1 << 0;
constexpr uint32_t QUADTREE_POSITIVE_Y = 0;
constexpr uint32_t QUADTREE_NEGATIVE_Y = 1 << 1;

static float2 QUADTREE_EXTENT_FACTORS[] = {
    float2{  1.f,  1.f },
    float2{ -1.f,  1.f },
    float2{  1.f, -1.f },
    float2{ -1.f, -1.f },
};

struct QuadtreeNode {
    QuadtreeNode(CountMemoryResource& memory_resource)
        : primitives(memory_resource)
    {
    }

    std::unique_ptr<QuadtreeNode> children[4];
    std::vector<AccelerationStructurePrimitive*, CountAllocator<AccelerationStructurePrimitive*>> primitives;
    aabbox2 bounds;
};

class QuadtreeAccelerationStructure : public AccelerationStructure, private QuadtreeNode {
public:
    QuadtreeAccelerationStructure(CountMemoryResource& memory_resource_, const float2& center, const float2& extent, uint32_t max_depth)
        : QuadtreeNode(memory_resource_)
        , memory_resource(memory_resource_)
        , m_max_depth(max_depth)
    {
        assert(extent.x > 0.f);
        assert(extent.y > 0.f);

        bounds.center = center;
        bounds.extent = extent;
    }

    void add(AccelerationStructurePrimitive& primitive) override {
        QuadtreeNode& node = find_node(primitive.get_bounds(), *this);
        assert(std::find(node.primitives.begin(), node.primitives.end(), &primitive) == node.primitives.end());

        node.primitives.push_back(&primitive);

        primitive.m_node = &node;
    }

    void remove(AccelerationStructurePrimitive& primitive) override {
        QuadtreeNode* node = static_cast<QuadtreeNode*>(primitive.m_node);
        assert(node != nullptr);

        auto it = std::find(node->primitives.begin(), node->primitives.end(), &primitive);
        assert(it != node->primitives.end());

        *it = node->primitives.back();
        node->primitives.pop_back();

        primitive.m_node = nullptr;
    }

    void update(AccelerationStructurePrimitive& primitive) override {
        QuadtreeNode* node = static_cast<QuadtreeNode*>(primitive.m_node);
        assert(node != nullptr);

        const aabbox3& bounds = primitive.get_bounds();

        if (bounds.center.x - bounds.extent.x <  node->bounds.center.x - node->bounds.extent.x ||
            bounds.center.z - bounds.extent.z <  node->bounds.center.y - node->bounds.extent.y ||
            bounds.center.x + bounds.extent.x >= node->bounds.center.x + node->bounds.extent.x ||
            bounds.center.z + bounds.extent.z >= node->bounds.center.y + node->bounds.extent.y)
        {
            auto it = std::find(node->primitives.begin(), node->primitives.end(), &primitive);
            assert(it != node->primitives.end());

            *it = node->primitives.back();
            node->primitives.pop_back();

            QuadtreeNode& node = find_node(bounds, *this);
            node.primitives.push_back(&primitive);

            primitive.m_node = &node;
        }
    }

    void query(const aabbox3& aabbox, std::vector<AccelerationStructurePrimitive*>& output) const override {
        collect_primitives(*this, aabbox, output);
    }

    void query(const frustum& frustum, std::vector<AccelerationStructurePrimitive*>& output) const override {
        float y0 = find_y(frustum.data[0], frustum.data[2], frustum.data[4]);
        float y1 = find_y(frustum.data[1], frustum.data[2], frustum.data[4]);
        float y2 = find_y(frustum.data[0], frustum.data[3], frustum.data[4]);
        float y3 = find_y(frustum.data[1], frustum.data[3], frustum.data[4]);
        float y4 = find_y(frustum.data[0], frustum.data[2], frustum.data[5]);
        float y5 = find_y(frustum.data[1], frustum.data[2], frustum.data[5]);
        float y6 = find_y(frustum.data[0], frustum.data[3], frustum.data[5]);
        float y7 = find_y(frustum.data[1], frustum.data[3], frustum.data[5]);

        float y_min = std::min({ y0, y1, y2, y3, y4, y5, y6, y7 });
        float y_max = std::max({ y0, y1, y2, y3, y4, y5, y6, y7 });

        float y_center = (y_max + y_min) / 2.f;
        float y_extent = (y_max - y_min) / 2.f;

        collect_primitives(*this, frustum, y_center, y_extent, output);
    }

private:
    QuadtreeNode& find_node(const aabbox3& bounds, QuadtreeNode& node, uint32_t depth = 0) {
        if (depth >= m_max_depth) {
            return node;
        }

        uint32_t index = 0;

        if (bounds.center.x - bounds.extent.x >= node.bounds.center.x) {
            index |= QUADTREE_POSITIVE_X;
        } else if (bounds.center.x + bounds.extent.x < node.bounds.center.x) {
            index |= QUADTREE_NEGATIVE_X;
        } else {
            return node;
        }

        if (bounds.center.z - bounds.extent.z >= node.bounds.center.y) {
            index |= QUADTREE_POSITIVE_Y;
        } else if (bounds.center.z + bounds.extent.z < node.bounds.center.y) {
            index |= QUADTREE_NEGATIVE_Y;
        } else {
            return node;
        }

        std::unique_ptr<QuadtreeNode>& child = node.children[index];
        if (!child) {
            float extent_x = node.bounds.extent.x / 2.f;
            float extent_y = node.bounds.extent.y / 2.f;

            float center_x = node.bounds.center.x + QUADTREE_EXTENT_FACTORS[index].x * extent_x;
            float center_y = node.bounds.center.y + QUADTREE_EXTENT_FACTORS[index].y * extent_y;

            child = std::unique_ptr<QuadtreeNode>(new (memory_resource.allocate(sizeof(QuadtreeNode))) QuadtreeNode(memory_resource));
            child->bounds = aabbox2{
                float2{ center_x, center_y },
                float2{ extent_x, extent_y }
            };
        }

        return find_node(bounds, *child, depth + 1);
    }

    float find_y(const plane& p1, const plane& p2, const plane& p3) const {
        float det = p1.normal.x * p2.normal.y * p3.normal.z +
                    p1.normal.y * p2.normal.z * p3.normal.x +
                    p1.normal.z * p2.normal.x * p3.normal.y -
                    p1.normal.z * p2.normal.y * p3.normal.x -
                    p1.normal.x * p2.normal.z * p3.normal.y -
                    p1.normal.y * p2.normal.x * p3.normal.z;

        float det_y = p1.normal.z * p2.distance * p3.normal.x +
                      p1.normal.x * p2.normal.z * p3.distance +
                      p1.distance * p2.normal.x * p3.normal.z -
                      p1.normal.x * p2.distance * p3.normal.z -
                      p1.distance * p2.normal.z * p3.normal.x -
                      p1.normal.z * p2.normal.x * p3.distance;

        return det_y / det;
    }
    
    void collect_primitives(const QuadtreeNode& node, const aabbox3& bounds, std::vector<AccelerationStructurePrimitive*>& output) const {
        output.reserve(output.size() + node.primitives.size());

        for (AccelerationStructurePrimitive* primitive : node.primitives) {
            if (intersect(primitive->get_bounds(), bounds)) {
                output.push_back(primitive);
            }
        }

        for (const std::unique_ptr<QuadtreeNode>& child : node.children) {
            if (child && intersect(child->bounds, bounds)) {
                collect_primitives(*child, bounds, output);
            }
        }
    }
    
    void collect_primitives(const QuadtreeNode& node, const frustum& bounds, float y_center, float y_extent, std::vector<AccelerationStructurePrimitive*>& output) const {
        output.reserve(output.size() + node.primitives.size());

        for (AccelerationStructurePrimitive* primitive : node.primitives) {
            if (intersect(primitive->get_bounds(), bounds)) {
                output.push_back(primitive);
            }
        }

        for (const std::unique_ptr<QuadtreeNode>& child : node.children) {
            if (child) {
                aabbox3 child_bounds{
                    float3{ child->bounds.center.x, y_center, child->bounds.center.y },
                    float3{ child->bounds.extent.x, y_extent, child->bounds.extent.y }
                };
                if (intersect(child_bounds, bounds)) {
                    collect_primitives(*child, bounds, y_center, y_extent, output);
                }
            }
        }
    }

    CountMemoryResource& memory_resource;
    uint32_t m_max_depth;
};
