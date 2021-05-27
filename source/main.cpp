#include "linear_acceleration_structure.h"
#include "octree_acceleration_structure.h"
#include "quadtree_acceleration_structure.h"

#include <chrono>
#include <iostream>
#include <random>

constexpr uint32_t MAX_DEPTH = 5;
constexpr size_t QUERY_COUNT = 1000;
constexpr size_t MIN_PRIMITIVES = 32;
constexpr size_t MAX_PRIMITIVES = 524288;

static std::mt19937 generator;
static std::uniform_real_distribution<float> center_distribution(-1024.f, 1024.f);
static std::uniform_real_distribution<float> extent_distribution(0.1f, 1.f);
static std::uniform_real_distribution<float> velocity_distribution(0.5f, 50.f);
static std::uniform_real_distribution<float> query_extent_distribution(2.f, 5.f);
static std::uniform_real_distribution<float> query_fov_distribution(1.047f, 2.269f);
static std::uniform_real_distribution<float> query_aspect_distribution(0.5f, 1.5f);
static std::uniform_real_distribution<float> query_near_distribution(0.01f, 0.5f);
static std::uniform_real_distribution<float> query_far_distribution(5.f, 50.f);

class TestPrimitive : public AccelerationStructurePrimitive {
public:
    TestPrimitive() {
        m_bounds.center.x = center_distribution(generator);
        m_bounds.center.y = center_distribution(generator);
        m_bounds.center.z = center_distribution(generator);
        m_bounds.extent.x = extent_distribution(generator);
        m_bounds.extent.y = extent_distribution(generator);
        m_bounds.extent.z = extent_distribution(generator);
        m_velocity.x      = velocity_distribution(generator);
        m_velocity.y      = velocity_distribution(generator);
        m_velocity.z      = velocity_distribution(generator);
    }

    void update(float elapsed_time) {
        m_bounds.center.x += m_velocity.x * elapsed_time;
        m_bounds.center.y += m_velocity.y * elapsed_time;
        m_bounds.center.z += m_velocity.z * elapsed_time;
    }

private:
    float3 m_velocity;
};

static void test_add(AccelerationStructure& acceleration_structure, std::vector<TestPrimitive>& primitives, bool print = true) {
    auto before = std::chrono::high_resolution_clock::now();

    for (TestPrimitive& primitive : primitives) {
        acceleration_structure.add(primitive);
    }

    auto after = std::chrono::high_resolution_clock::now();

    if (print) {
        std::cout << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() / 1000000.0;
    }
}

static void test_update(AccelerationStructure& acceleration_structure, std::vector<TestPrimitive>& primitives) {
    for (TestPrimitive& primitive : primitives) {
        primitive.update(0.0167f);
    }

    auto before = std::chrono::high_resolution_clock::now();

    for (TestPrimitive& primitive : primitives) {
        acceleration_structure.update(primitive);
    }

    auto after = std::chrono::high_resolution_clock::now();

    std::cout << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() / 1000000.0;
}

static aabbox3 aabboxes[QUERY_COUNT];

// Model is written by linear acceleration structure, check is written by other acceleration structures. Check must be equal to model.
static std::vector<AccelerationStructurePrimitive*> aabbox_model[QUERY_COUNT];
static std::vector<AccelerationStructurePrimitive*> aabbox_check[QUERY_COUNT];

static void test_query_aabbox(AccelerationStructure& acceleration_structure, size_t n, bool check) {
    if (check) {
        for (std::vector<AccelerationStructurePrimitive*>& check : aabbox_check) {
            check.clear();
        }
    } else {
        for (std::vector<AccelerationStructurePrimitive*>& model : aabbox_model) {
            model.clear();
        }
    }

    auto before = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < QUERY_COUNT; i++) {
        acceleration_structure.query(aabboxes[i], check ? aabbox_check[i] : aabbox_model[i]);
    }

    auto after = std::chrono::high_resolution_clock::now();

    std::cout << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() / 1000000.0 / QUERY_COUNT;

    if (check) {
        for (size_t i = 0; i < QUERY_COUNT; i++) {
            if (aabbox_check[i].size() != aabbox_model[i].size()) {
                std::cout << "AABBox query sizes don't match." << std::endl;
                std::abort();
            }

            std::sort(aabbox_check[i].begin(), aabbox_check[i].end());

            for (size_t j = 0; j < aabbox_model[i].size(); j++) {
                if (aabbox_check[i][j] != aabbox_model[i][j]) {
                    std::cout << "AABBox query primitives don't match." << std::endl;
                    std::abort();
                }
            }
        }
    } else {
        for (std::vector<AccelerationStructurePrimitive*>& model : aabbox_model) {
            std::sort(model.begin(), model.end());
        }
    }
}

static frustum frustums[QUERY_COUNT];

// Model is written by linear acceleration structure, check is written by other acceleration structures. Check must be equal to model.
static std::vector<AccelerationStructurePrimitive*> frustum_model[QUERY_COUNT];
static std::vector<AccelerationStructurePrimitive*> frustum_check[QUERY_COUNT];

static void test_query_frustum(AccelerationStructure& acceleration_structure, size_t n, bool check) {
    if (check) {
        for (std::vector<AccelerationStructurePrimitive*>& check : frustum_check) {
            check.clear();
        }
    } else {
        for (std::vector<AccelerationStructurePrimitive*>& model : frustum_model) {
            model.clear();
        }
    }

    auto before = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < QUERY_COUNT; i++) {
        acceleration_structure.query(frustums[i], check ? frustum_check[i] : frustum_model[i]);
    }

    auto after = std::chrono::high_resolution_clock::now();

    std::cout << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() / 1000000.0 / QUERY_COUNT;

    if (check) {
        for (size_t i = 0; i < QUERY_COUNT; i++) {
            if (frustum_check[i].size() != frustum_model[i].size()) {
                std::cout << "Frustum query sizes don't match." << std::endl;
                std::abort();
            }

            std::sort(frustum_check[i].begin(), frustum_check[i].end());

            for (size_t j = 0; j < frustum_model[i].size(); j++) {
                if (frustum_check[i][j] != frustum_model[i][j]) {
                    std::cout << "Frustum query primitives don't match." << std::endl;
                    std::abort();
                }
            }
        }
    } else {
        for (std::vector<AccelerationStructurePrimitive*>& model : frustum_model) {
            std::sort(model.begin(), model.end());
        }
    }
}

static void test_remove(AccelerationStructure& acceleration_structure, std::vector<TestPrimitive>& primitives) {
    std::vector<TestPrimitive*> shuffled_primitives(primitives.size());
    for (size_t i = 0; i < shuffled_primitives.size(); i++) {
        shuffled_primitives[i] = &primitives[i];
    }
    std::shuffle(shuffled_primitives.begin(), shuffled_primitives.end(), generator);

    auto before = std::chrono::high_resolution_clock::now();

    for (TestPrimitive* primitive : shuffled_primitives) {
        acceleration_structure.remove(*primitive);
    }

    auto after = std::chrono::high_resolution_clock::now();

    std::cout << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() / 1000000.0;
}

static void test(AccelerationStructure& acceleration_structure, std::vector<TestPrimitive>& primitives, bool check) {
    generator = std::mt19937();

    for (TestPrimitive& primitive : primitives) {
        // Primitives are updated during each test, we want exactly input though (including addresses).
        primitive = TestPrimitive();
    }

    test_add(acceleration_structure, primitives);
    test_update(acceleration_structure, primitives);
    test_query_aabbox(acceleration_structure, primitives.size(), check);
    test_query_frustum(acceleration_structure, primitives.size(), check);
    test_remove(acceleration_structure, primitives);
}

static void test_linear_acceleration_structure(std::vector<TestPrimitive>& primitives) {
    CountMemoryResource memory_resource;
    LinearAccelerationStructure acceleration_structure(memory_resource);
    test(acceleration_structure, primitives, false);
    std::cout << " " << memory_resource.allocated;
}

static void test_octree_acceleration_structure(std::vector<TestPrimitive>& primitives) {
    CountMemoryResource memory_resource;
    OctreeAccelerationStructure acceleration_structure(memory_resource, float3{}, float3{ 1024.f, 1024.f, 1024.f }, MAX_DEPTH);
    test(acceleration_structure, primitives, true);
    std::cout << " " << memory_resource.allocated;
}

static void test_quadtree_acceleration_structure(std::vector<TestPrimitive>& primitives) {
    CountMemoryResource memory_resource;
    QuadtreeAccelerationStructure acceleration_structure(memory_resource, float2{}, float2{ 1024.f, 1024.f }, MAX_DEPTH);
    test(acceleration_structure, primitives, true);
    std::cout << " " << memory_resource.allocated;
}

int main(int argc, char* argv[]) {
    for (aabbox3& aabbox : aabboxes) {
        aabbox.center.x = center_distribution(generator);
        aabbox.center.y = center_distribution(generator);
        aabbox.center.z = center_distribution(generator);
        aabbox.extent.x = query_extent_distribution(generator);
        aabbox.extent.y = query_extent_distribution(generator);
        aabbox.extent.z = query_extent_distribution(generator);
    }

    for (frustum& frustum : frustums) {
        float3 source;
        source.x = center_distribution(generator);
        source.y = center_distribution(generator);
        source.z = center_distribution(generator);

        float3 target;
        target.x = center_distribution(generator);
        target.y = center_distribution(generator);
        target.z = center_distribution(generator);

        float3 up;
        up.x = 0.f;
        up.y = 1.f;
        up.z = 0.f;

        float4x4 view = look_at(source, target, up);

        float fov = query_fov_distribution(generator);
        float aspect = query_aspect_distribution(generator);
        float z_near = query_near_distribution(generator);
        float z_far = query_far_distribution(generator);

        float4x4 projection = perspective(fov, aspect, z_near, z_far);

        float4x4 view_projection = mul(view, projection);

        frustum = frustum_from_float4x4(view_projection);
    }

    for (std::vector<AccelerationStructurePrimitive*>& model : aabbox_model) {
        model.reserve(MAX_PRIMITIVES);
    }

    for (std::vector<AccelerationStructurePrimitive*>& check : aabbox_check) {
        check.reserve(MAX_PRIMITIVES);
    }

    for (std::vector<AccelerationStructurePrimitive*>& model : frustum_model) {
        model.reserve(MAX_PRIMITIVES);
    }

    for (std::vector<AccelerationStructurePrimitive*>& check : frustum_check) {
        check.reserve(MAX_PRIMITIVES);
    }

    // Perform this test multiple times, use average for statistics.
    for (size_t i = 0; i < 5; i++) {
        for (size_t n = MIN_PRIMITIVES; n <= MAX_PRIMITIVES; n *= 2) {
            std::cout << n;

            // Primitive addresses must be the same for all acceleration structures.
            std::vector<TestPrimitive> primitives(n);

            test_linear_acceleration_structure(primitives);
            test_octree_acceleration_structure(primitives);
            test_quadtree_acceleration_structure(primitives);

            std::cout << std::endl;
        }
    }

    return 0;
}
